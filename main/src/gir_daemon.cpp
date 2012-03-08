#include <sys/stat.h>
#include <sstream>
#include <string.h>
#include <GIRConfig.h>
#include <GIRServer.h>
#include <GIRLogger.h>
#include <MRIDataComm.h>
#include <GIRXML.h>
#include <stdio.h>
#include <sys/wait.h>

//DEV

// reap zombies
void sigchld_handler( int s ) { while( waitpid( -1, NULL, WNOHANG ) > 0 ); }

int main( int argc, char** argv ) {
	// load config file
	std::string config_path = "";
	if( argc > 1 )
		config_path = argv[1];
	else
	{
		std::string home_dir = getenv( "HOME" );
		GIRUtils::CompleteDirPath( home_dir );
		config_path = home_dir + ".gir_config.xml";
	}
	GIRConfig config;
	if( !GIRXML::Load( config_path.c_str(), config ) )
	{
		fprintf( stderr, "Unable to load config_file: \"%s\", daemon aborting!\n", config_path.c_str() );
		exit( EXIT_FAILURE );
	}

	// create the gir server and configure it
	GIRServer gir_server;
	if( !gir_server.Configure( config, true, true ) )
	{
		fprintf( stdout, "Could not configure GIR, daemon aborting!\n" );
		exit( EXIT_FAILURE );
	}

	// start logging
	if( !GIRLogger::Instance()->LogToFile( gir_server.LogPath().c_str() ) )
	{
		fprintf( stderr, "Unable to open log file: \"%s,\" daemon aborting!\n", gir_server.LogPath().c_str() );
		exit( EXIT_FAILURE );
	}
	printf( "logging to: %s\n", gir_server.LogPath().c_str() );

	// fork off the daemon and close parent
	pid_t pid = fork();
	if( pid < 0 )
	{
		fprintf( stderr, "Problem with fork(), daemon aborting!\n" );
		exit( EXIT_FAILURE );
	}
	if( pid > 0 )
		exit( EXIT_SUCCESS );

	// reset umask
	umask( 0 );

	// create new sid
	pid_t sid = setsid();
	if( sid < 0 )
	{
		fprintf( stderr, "Could not create process group, daemon aborting!\n" );
		exit( EXIT_FAILURE );
	}

	// chdir to /
	if( chdir( "/" ) < 0 )
	{
		fprintf( stderr, "Could not change working directory to /, daemon aborting!\n" );
		exit( EXIT_FAILURE );
	}

	// start listening
	if( !gir_server.StartListening() )
	{
		GIRLogger::LogError( "StartListening failed for GIR server!\n" );
		exit( EXIT_FAILURE );
	}

	// redirect standard file descriptors
	//freopen( "/dev/null", "r", stdin );
	//freopen( "/dev/null", "w", stdout );
	//freopen( "/dev/null", "w", stderr );

	gir_server.PrintGIR();
	GIRLogger::LogInfo( "GIR settings:\n%s", gir_server.GetConfigString().c_str() );

	// zombies == bad
	struct sigaction sa;
	sa.sa_handler = sigchld_handler;
	sigemptyset( &sa.sa_mask );
	sa.sa_flags = SA_RESTART;
	if( sigaction( SIGCHLD, &sa, NULL ) == -1 )
	{
		fprintf( stderr, "\n" );
		perror( "sigaction" );
		fprintf( stderr, "\n" );
		exit( EXIT_FAILURE );
	}

	// process connections
	while( true )
	{
		// accept next connection
		if( gir_server.AcceptConnection() )
		{
			// fork of child process to handle the connection
			if( !fork() )
			{
				try
				{
					// child doesn't need this socket
					gir_server.StopListening();

					// process
					GIRLogger::LogInfo( "client connected...\n" );
					gir_server.ProcessRequest( config );

					// disconnect
					GIRLogger::LogInfo( "disconnecting from client\n" );
					gir_server.CloseConnection();
				}
				catch( const char* exc ) { GIRLogger::LogError( "GIR server threw an exception (char*): \"%s\"!\n", exc ); }
				catch( const exception& exc ) { GIRLogger::LogError( "GIR server threw an exception (exception) \"%s\"!\n", exc.what() ); }
				catch( std::string exc ) { GIRLogger::LogError( "GIR server threw an exception (std::string): \"%s\"!\n", exc.c_str() ); }
				catch( ... ) { GIRLogger::LogError( "GIR server threw an exception!\n" ); }
				exit( EXIT_SUCCESS );
			}
			// parent doesn't need this socket
			else
				gir_server.CloseConnection();
		}
		else
			GIRLogger::LogError( "Couldn't accept connection!\n" );
	}
	
	// never executes...
	exit( EXIT_SUCCESS );
}
