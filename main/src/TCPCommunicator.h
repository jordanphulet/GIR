#ifndef __TCP_DATA_COMMUNICATOR_H__
#define __TCP_DATA_COMMUNICATOR_H__

#include "DataCommunicator.h"

class MRIData;

class TCPCommunicator: public DataCommunicator
{
	public:
	TCPCommunicator( int new_backlog = 10, int new_buffer_size = 1024000 );
	virtual ~TCPCommunicator();

	bool Listen( int port );
	void StopListening();
	bool AcceptConnection();
	bool Connect( const char* server, int port );
	void CloseConnection();

	bool Connected() const { return connected; }
	bool Listening() const { return listening; }

	virtual bool SendData( MRIData& mri_data );

	protected:
	virtual int SendAll( char* data, int data_length );
	virtual int ReceiveAll( char* data, int data_length );

	private:
	int listen_sock_fd;
	int sock_fd;
	bool connected;
	bool listening;
	const int backlog;
};

#endif

