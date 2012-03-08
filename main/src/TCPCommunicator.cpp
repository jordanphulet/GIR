#ifndef WIN32
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h> 
#else
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <stdio.h>

	#define close closesocket
#endif

#include <errno.h>

#include "TCPCommunicator.h"
#include "GIRLogger.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>



TCPCommunicator::TCPCommunicator( int new_backlog, int new_buffer_size ) :
	DataCommunicator( new_buffer_size ),
	connected( false ), 
	listening( false ), 
	backlog( new_backlog )
{

#ifdef WIN32
	WSADATA wsa_data;
    if( WSAStartup( MAKEWORD( 2, 0 ), &wsa_data ) != 0 )
        GIRLogger::LogError( "WSAStartup failed, sockets will not work!\n" );
#endif
	
}

TCPCommunicator::~TCPCommunicator()
{
	if( Listening() )
		StopListening();
	if( Connected() )
		CloseConnection();
}

bool TCPCommunicator::Listen( int port )
{
	if( Listening() )
	{
		GIRLogger::LogError( "TCPCommunicator::Listen -> already listening!\n" );
		return false;
	}

	// load up address info
	struct addrinfo hints, *res;
	memset( &hints, 0, sizeof hints );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	std::stringstream port_stream;
	port_stream << port;
	getaddrinfo( NULL, port_stream.str().c_str(), &hints, &res );

	// make a socket, bind it, and listen on it
	listen_sock_fd = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
	if( bind( listen_sock_fd, res->ai_addr, res->ai_addrlen ) < 0 )
	{
		perror( "Socket binding error" );
		return false;
	}
	listen( listen_sock_fd, backlog );

	//int optval = 1;
	//setsockopt( listen_sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval );
	listening = true;
	//GIRLogger::LogInfo( "listening on port %d...\n", port );

	return true;
}

void TCPCommunicator::StopListening()
{
	if( Listening() )
	{
		close( listen_sock_fd );
		listening = false;
	}
}

bool TCPCommunicator::AcceptConnection()
{
	if( Connected() )
	{
		GIRLogger::LogError( "TCPCommunicator::AcceptConnection -> accepting another connection while still connected to another host, closing previous connection!" );
		CloseConnection();
	}

	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof their_addr;
	sock_fd = accept(listen_sock_fd, (struct sockaddr *)&their_addr, &addr_size);
	if( sock_fd == -1 )
		perror( "TCPCommunicator::AcceptConnection() -> aborting, problem with accept" );
	else
		connected = true;
	return Connected();
}

bool TCPCommunicator::Connect( const char* server, int port )
{
	if( port < 0 || port > 65535 )
	{
		GIRLogger::LogError( "TCPCommunicator::Connect -> specified port: %d, out of range!\n", port );
		return false;
	}

	if( Connected() )
	{
		GIRLogger::LogError( "TCPCommunicator::Connect -> already connected!\n" );
		return false;
	}

	if( Listening() )
	{
		GIRLogger::LogError( "TCPCommunicator::Connect -> already listening for other connections, so can't Connect()!\n" );
		return false;
	}

	std::stringstream stream;
	stream << port;

	// get server information
	struct addrinfo hints, *server_info;
	memset( &hints, 0, sizeof hints );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo( server, stream.str().c_str(), &hints, &server_info);

	// loop through all the results and connect to the first we can
	struct addrinfo* p;
	for( p = server_info; p != NULL; p = p->ai_next )
	{
		if( (sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1 )
			continue;

		if( connect( sock_fd, p->ai_addr, p->ai_addrlen ) == -1 )
		{
			close(sock_fd);
			continue;
		}
		break;
	}

	if( p == 0 )
	{
		GIRLogger::LogError( "TCPCommunicator::Connect -> unable to connect!\n" );
		return false;
	}
	
	connected = true;
	//GIRLogger::LogInfo( "connected\n" );
	return true;
}

void TCPCommunicator::CloseConnection()
{
	if( Connected() )
	{
		close( sock_fd );
		connected = false;
	}
} 

bool TCPCommunicator::SendData( MRIData& mri_data )
{
	return DataCommunicator::SendData( mri_data );
}

int TCPCommunicator::SendAll( char* data, int data_length )
{
	if( !Connected() )
	{
		GIRLogger::LogError( "TCPCommunicator::SendAll -> can't send all if not connected!\n" );
		return -1;
	}

	int bytes_sent = 0;
	int n = 0;
	while( bytes_sent < data_length ) {
//#ifndef WIN32
//		n = send( sock_fd, data+bytes_sent, data_length-bytes_sent, MSG_NOSIGNAL ); 
//#else
		n = send( sock_fd, data+bytes_sent, data_length-bytes_sent, 0 ); 
//#endif
		if( n < 0 )
			break;
		bytes_sent += n;
	}

	if( bytes_sent < 1 || n == -1 )
	{
		GIRLogger::LogError( "TCPCommunicator::SendAll -> error with send(), n: %d, data_length: %d, bytes_sent: %d!\n", n, data_length, bytes_sent );
		perror( "send" );
		CloseConnection();
		return -1;
	}

	return bytes_sent;
}

int TCPCommunicator::ReceiveAll( char* buffer, int data_length )
{
	if( !Connected() )
	{
		GIRLogger::LogError( "TCPCommunicator::ReceiveAll -> can't receive all if not connected!\n" );
		return -1;
	}

	int bytes_received = 0;
	int n = 0;
	while( bytes_received < data_length )
	{
		n = recv( sock_fd, buffer+bytes_received, data_length-bytes_received, 0 );
		if( n < 1 )
		{
			if( errno == EINTR )
			{
				GIRLogger::LogWarning( "TCPCommunicator::ReceiveAll -> interrupted system call, trying again...\n" );
				continue;
			}
			GIRLogger::LogError( "TCPCommunicator::ReceiveAll -> error number: %d!\n", errno );
			perror( "recv" );
			break;
		}
		bytes_received += n;
	}

	if( bytes_received < 1 || n == -1 )
	{
		CloseConnection();
		GIRLogger::LogError( "TCPCommunicator::ReceiveAll -> error with recv()!\n" );
		return -1;
	}

	return bytes_received;
}
