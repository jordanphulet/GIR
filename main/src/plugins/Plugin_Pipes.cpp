#include <GIRConfig.h>
#include <GIRLogger.h>
#include <GIRUtils.h>
#include <MRIDataComm.h>
#include <FileCommunicator.h>
#include <plugins/Plugin_Pipes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>

extern "C" ReconPlugin* create( const char* alias )
{
	return new Plugin_Pipes( "Plugin_Pipes", alias );
}

extern "C" void destroy( ReconPlugin* plugin )
{
	delete plugin;
}

bool Plugin_Pipes::Configure( GIRConfig& config, bool main_config, bool final_config )
{
	if( main_config )
	{
		// get pipe_prog_dir
		if( config.GetParam( plugin_id.c_str(), alias.c_str(), "pipe_prog_dir", pipe_prog_dir ) )
			GIRUtils::CompleteDirPath( pipe_prog_dir );
	}
	else
	{
		// pipe_prog_dir can only be set in the main config
		std::string desired_pipe_prog_dir;
		if( config.GetParam( plugin_id.c_str(), alias.c_str(), "pipe_prog_dir", desired_pipe_prog_dir ) )
		{
			GIRLogger::LogError( "Plugin_Pipes::Configure -> pipe_prog_dir can only by set in the main config!\n" );
			return false;
		}
	}

	// get pipe_prog 
	if( config.GetParam( plugin_id.c_str(), alias.c_str(), "pipe_prog", pipe_prog ) )
	{
		// relative paths are not allowed
		if( pipe_prog.find_last_of( "/" ) != pipe_prog.npos )
		{
			GIRLogger::LogError( "Plugin_Plugin::Configure -> invalid character '/' in pipe_prog!\n" );
			return false;
		}
	}

	return true;
}

bool Plugin_Pipes::Reconstruct( MRIData& mri_data )
{
	GIRLogger::LogDebug( "### pipes reconstructing...\n" );

	int pid = (int)getpid();

	GIRLogger::LogDebug( "### pid: %d...\n", pid );

	std::stringstream stream1;
	std::stringstream stream2;
	stream1 << "/tmp/gir_fifo1." << pid;
	stream2 << "/tmp/gir_fifo2." << pid;
	std::string fifo1_path = stream1.str();
	std::string fifo2_path = stream2.str();

	GIRLogger::LogDebug( "### mkfifo...\n" );
	// make fifos
	if( mkfifo( fifo1_path.c_str(), 0666 ) == -1 || mkfifo( fifo2_path.c_str(), 0666 ) == -1 )
	{
		GIRLogger::LogError( "Plugin_Pipes::Reconstruct -> unable to create fifos!\n" );
		return false;
	}

	// set environment variables
	setenv( "GIR_FIFO1", fifo1_path.c_str(), 1 );
	setenv( "GIR_FIFO2", fifo2_path.c_str(), 1 );

	GIRLogger::LogDebug( "### fifos: %s %s\n", fifo1_path.c_str(), fifo2_path.c_str() );

	// fork()
	//TODO: do I need to wait()
	pid_t fork_pid = fork();
	if( fork_pid == -1 )
	{
		GIRLogger::LogError( "Plugin_Pipes::Reconstruct -> fork() failed!\n" );
		return false;
	}

	// child
	if( fork_pid == 0 )
	{
		std::string command = pipe_prog_dir + pipe_prog;
		GIRLogger::LogDebug( "### executing command: %s\n", command.c_str() );
		execl( command.c_str(), pipe_prog.c_str(), alias.c_str(), (char*)0 );
		GIRLogger::LogDebug( "### child done\n" );
		exit( EXIT_SUCCESS );
	}
	// parent
	else
	{
		// create file communicator to communicate with child
		FileCommunicator communicator;
		if( !communicator.OpenOutput( fifo1_path.c_str() ) || !communicator.OpenInput( fifo2_path.c_str() ) )
		{
			GIRLogger::LogError( "Plugin_Pipes::Reconstruct -> could not open file descriptors!\n" );
			return false;
		}
		GIRLogger::LogDebug( "### parent fifos opened\n" );

		// send config
		GIRConfig config;
		if( !communicator.SendConfig( config ) )
		{
			GIRLogger::LogError( "Plugin_Pipes::Reconstruct -> communicator.SendConfig() failed!\n" );
			return false;
		}

		// send mri_data
		if( !communicator.SendData( mri_data ) )
		{
			GIRLogger::LogError( "Plugin_Pipes::Reconstruct -> communicator.SendData() failed!\n" );
			return false;
		}

		// get ack
		MRIReconAck ack;
		if( !communicator.ReceiveReconAck( ack ) )
		{
			GIRLogger::LogError( "Plugin_Pipes::Reconstruct -> communicator.ReceiveReconAck() failed!\n" );
			return false;
		}

		if( !ack.success )
		{
			GIRLogger::LogError( "Plugin_Pipes::Reconstruct -> ack did not succeed!\n" );
			return false;
		}

		// get data
		if( !communicator.ReceiveData( mri_data ) )
		{
			GIRLogger::LogError( "Plugin_Pipes::Reconstruct -> communicator.ReceiveData() failed!\n" );
			return false;
		}
	}

	// remove fifos
	remove( fifo1_path.c_str() );
	remove( fifo2_path.c_str() );

	// close pipes
	return true;
}

bool Plugin_Pipes::Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data )
{
	GIRLogger::LogError( "PluginPipes::Reconstruct -> reconstruction with MRIMeasurement vector not implemented!\n" );
	return false;
}
