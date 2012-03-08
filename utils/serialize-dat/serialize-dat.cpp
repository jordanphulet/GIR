#include <stdio.h>
#include <cstdlib>
#include <vector>
#include <MRIData.h>
#include <SiemensTool.h>
#include <Serializable.h>
#include <DataSorter.h>
#include <GIRLogger.h>
#include <FileCommunicator.h>

int main( int argc, char** argv )
{
	if( argc != 2 )
	{
		printf( "usage: serialize-dat DAT_FILE\n" );
		exit( EXIT_FAILURE );
	}

	const char* dat_file = argv[1];

	// don't want output goin to file...
	GIRLogger::Instance()->silent = true;

	// load dat file
	MRIDataHeader header;
	std::vector<MRIMeasurement> meas_vector;
	if( ! SiemensTool::LoadDatFile( dat_file, header, meas_vector ) )
	{
		fprintf( stderr, "unable to load dat file: %s, aborting!\n", argv[1] );
		exit( EXIT_FAILURE );
	}

	// create file communicator with output to stdout
	FileCommunicator communicator;
	communicator.OpenOutputFD( STDOUT_FILENO );

	// send request
	MRIReconRequest request;
	request.pipeline = "nothing";
	communicator.SendReconRequest( request );

	// send header
	communicator.SendDataHeader( header );

	// send measurements
	std::vector<MRIMeasurement>::iterator it;
	for( it = meas_vector.begin(); it != meas_vector.end(); it++ )
		communicator.SendMeasurement( *it );
	communicator.SendEndSignal();

	exit( EXIT_SUCCESS );
}
