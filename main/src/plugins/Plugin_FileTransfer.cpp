#include <GIRConfig.h>
#include <GIRLogger.h>
#include <GIRUtils.h>
#include <FileCommunicator.h>
#include <plugins/Plugin_FileTransfer.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>

extern "C" ReconPlugin* create( const char* alias )
{
	return new Plugin_FileTransfer( "Plugin_FileTransfer", alias );
}

extern "C" void destroy( ReconPlugin* plugin )
{
	delete plugin;
}

bool Plugin_FileTransfer::Configure( GIRConfig& config, bool main_config, bool final_config )
{
	if( main_config )
	{
		// get ext_prog_dir
		if( config.GetParam( 0, 0, "ext_prog_dir", ext_prog_dir ) )
			GIRUtils::CompleteDirPath( ext_prog_dir );
	}
	else
	{
		// ext_prog_dir can only be set in the main config
		std::string desired_ext_prog_dir;
		if( config.GetParam( plugin_id.c_str(), alias.c_str(), "ext_prog_dir", desired_ext_prog_dir ) )
		{
			GIRLogger::LogError( "Plugin_FileTransfer::Configure -> ext_prog_dir can only by set in the main config!\n" );
			return false;
		}
	}

	// get ext_prog 
	if( config.GetParam( plugin_id.c_str(), alias.c_str(), "ext_prog", ext_prog ) )
	{
		// relative paths are not allowed
		if( ext_prog.find_last_of( "/" ) != ext_prog.npos )
		{
			GIRLogger::LogError( "Plugin_FileTransfer::Configure -> invalid character '/' in ext_prog!\n" );
			return false;
		}
	}

	// make sure we can read the ext_prog
	string full_ext_prog_path = ext_prog_dir + ext_prog;
	if( final_config && !GIRUtils::FileIsReadable( full_ext_prog_path.c_str() ) )
	{
		GIRLogger::LogError( "Plugin_FileTransfer::Configure -> specified external program: %s could not be read!\n", full_ext_prog_path.c_str() );
		return false;
	}

	accum_config.Add( config );

	return true;
}

bool Plugin_FileTransfer::Reconstruct( MRIData& mri_data )
{
	// get io file paths
	int pid = (int)getpid();
	std::stringstream stream1;
	std::stringstream stream2;
	stream1 << "/uufs/chpc.utah.edu/common/home/u0236403/gir/gir_input." << pid;
	stream2 << "/uufs/chpc.utah.edu/common/home/u0236403/gir/gir_output." << pid;
	std::string ext_input_path = stream1.str();
	std::string ext_output_path = stream2.str();

	// set environment variables
	setenv( "GIR_PLUGIN_ALIAS", alias.c_str(), 1 );
	setenv( "GIR_EXT_INPUT", ext_input_path.c_str(), 1 );
	setenv( "GIR_EXT_OUTPUT", ext_output_path.c_str(), 1 );

	// create file communicator to communicate with child
	FileCommunicator communicator;
	if( !communicator.OpenOutput( ext_input_path.c_str() ) )
	{
		GIRLogger::LogError( "Plugin_FileTransfer::Reconstruct -> could not open ext_input file: '%s'!\n", ext_input_path.c_str() );
		return false;
	}

	// send config
	if( !communicator.SendConfig( accum_config ) )
	{
		GIRLogger::LogError( "Plugin_FileTransfer::Reconstruct -> communicator.SendConfig() failed!\n" );
		return false;
	}

	// send mri_data
	if( !communicator.SendData( mri_data ) )
	{
		GIRLogger::LogError( "Plugin_FileTransfer::Reconstruct -> communicator.SendData() failed!\n" );
		return false;
	}

	// run external program
	std::string command = ext_prog_dir + ext_prog;
	GIRLogger::LogDebug( "### new running external program: %s...\n", command.c_str() );
	//execl( command.c_str(), ext_prog.c_str(), alias.c_str(), (char*)0 );
	system( command.c_str() );

	GIRLogger::LogDebug( "### external program finished\n" );

	// open file created by external program
	if( !communicator.OpenInput( ext_output_path.c_str() ) )
	{
		GIRLogger::LogError( "Plugin_FileTransfer::Reconstruct -> could not open ext_output file: '%s'!\n", ext_output_path.c_str() );
		return false;
	}

	// get ack
	MRIReconAck ack;
	if( !communicator.ReceiveReconAck( ack ) )
	{
		GIRLogger::LogError( "Plugin_FileTransfer::Reconstruct -> communicator.ReceiveReconAck() failed!\n" );
		return false;
	}

	if( !ack.success )
	{
		GIRLogger::LogError( "Plugin_FileTransfer::Reconstruct -> ack did not succeed!\n" );
		return false;
	}

	// get data
	if( !communicator.ReceiveData( mri_data ) )
	{
		GIRLogger::LogError( "Plugin_FileTransfer::Reconstruct -> communicator.ReceiveData() failed!\n" );
		return false;
	}

	// remove files
	//remove( ext_input_path.c_str() );
	//remove( ext_output_path.c_str() );

	GIRLogger::LogDebug( "### success!!!\n" );
	return true;
}
