#include <MRIData.h>
#include <TCPCommunicator.h>
//#include <Serializable.h>
#include <MRIDataComm.h>
#include <SiemensTool.h>
#include <matlab/MexData.h>
#include <string>
#include <sstream>
#include <mex.h> 
#include <stdio.h>
#include <vector>

void PrintUsage() {
	mexPrintf( "Usage:\n" );
	mexPrintf( "    SendDat( dat_file_path, ip_address, port, recon_pipeline, [config_keys], [config_values] )\n" );
	mexPrintf( "    SendDat( data_matrix, ip_address, port, recon_pipeline, [config_keys], [config_values] )\n" );
}


bool CheckArgs( int nlhs, int nrhs, const mxArray* prhs[] )
{
	// check for correct number of arguments
	if( nrhs < 4 && nrhs != 6 )
	{
		mexPrintf( "wrong number of arguments!\n" );
		return false;
	}

	// make sure port wasn't specified as a string
	if( mxArrayToString( prhs[2] ) != 0 )
	{
		mexPrintf( "port cannot be a string!\n" );
		return false;
	}
	return true;
}

bool FillConfig( GIRConfig& config, const mxArray& keys, const mxArray& values )
{
	mwSize keys_size = mxGetNumberOfDimensions( &keys );
	mwSize values_size = mxGetNumberOfDimensions( &values );
	const mwSize* keys_dims = mxGetDimensions( &keys );
	const mwSize* values_dims = mxGetDimensions( &values );

	if( keys_size != 2 || keys_dims[0] != 1 || values_size != 2 || values_dims[0] != 1 || keys_dims[1] != values_dims[1] )
	{
		mexPrintf( "key and value arrays must both be 1xN and the same size!\n" );
		return false;
	}
	for( int i = 0; i < keys_dims[1]; i++ )
	{
		const mxArray* key_cell = mxGetCell( &keys, i );
		const mxArray* value_cell = mxGetCell( &values, i );
		if( key_cell == 0 || value_cell == 0 )
		{
			mexPrintf( "keys and values must be the same size!\n" );
			return false;
		}
		char* key = mxArrayToString( key_cell );
		char* value = mxArrayToString( value_cell );
		if( key == 0 || value == 0 )
		{
			mexPrintf( "keys and values must be strings!\n" );
			return false;
		}
		
		// get id, alias, and param name from key
		{
			std::stringstream key_stream( key );
			std::vector<std::string> elements;
			std::string item;
			while( std::getline( key_stream, item, ':' ) )
				elements.push_back( item );

			if( elements.size() != 3 )
			{
				mexPrintf( "Invalid config key: %s!\n", key );
				return false;
			}
			else if( !config.SetParam( elements[0].c_str(), elements[1].c_str(), elements[2].c_str(), value ) )
			{
				mexPrintf( "Unable to set config param, key: %s, value: %s!\n", key, value );
				return false;
			}
		}
	}

	return true;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	// make sure arguments are valid
	if( !CheckArgs( nlhs, nrhs, prhs ) )
	{
		PrintUsage();
		return;
	}

	char* dat_file_path = mxArrayToString( prhs[0] );
	char* ip_address = mxArrayToString( prhs[1] );
	int port = (int)mxGetScalar( prhs[2] );
	char* recon_pipeline = mxArrayToString( prhs[3] );

	if( ip_address == 0 || recon_pipeline == 0 )
		mexErrMsgTxt( "ip_address and recon_pipeline need to be strings.\n" );

	// fill config
	GIRConfig config;
	if( nrhs == 6 && !FillConfig( config, *prhs[4], *prhs[5] ) )
		mexErrMsgTxt( "Unable to fill config!\n" );
	printf( "config: %s\n", config.ToString().c_str() );

	// connect
	mexPrintf( "Connecting to %s:%d...\n", ip_address, port );
	TCPCommunicator communicator;
	if( !communicator.Connect( ip_address, port ) )
		mexErrMsgTxt( "communicator.Connect() failed!\n" );

	// send recon request
	mexPrintf( "\tsending request...\n" );
	MRIReconRequest request;
	request.pipeline = recon_pipeline;
	request.config = config;
	mexPrintf( "%s\n", request.ToString().c_str() );
	communicator.SendReconRequest( request );

	// send dat file
	if( dat_file_path != 0 )
	{
		// load data
		mexPrintf( "\tloading dat file...\n" );
		MRIDataHeader header;
		std::vector<MRIMeasurement> meas_vector;
		if( !SiemensTool::LoadDatFile( dat_file_path, header, meas_vector ) )
			mexErrMsgTxt( "Loading dat file failed!\n" );
	
		// send header
		mexPrintf( "\tsending header...\n" );
		communicator.SendDataHeader( header );
	
		// send measurements
		mexPrintf( "\tsending measurements...\n" );
		std::vector<MRIMeasurement>::iterator it;
		for( it = meas_vector.begin(); it != meas_vector.end(); it++ )
			communicator.SendMeasurement( *it );
		communicator.SendEndSignal();
	}
	// send matrix
	else
	{
		MRIData data;
		if( !MexData::ImportMexArray( data, prhs[0], true ) )
			mexErrMsgTxt( "MexData::ImportMexArray failed!\n" );

		mexPrintf( "\tsending data matrix...\n" );
		if( !communicator.SendData( data ) )
			mexErrMsgTxt( "Sending data failed!\n" );
	}

	// get ack
	mexPrintf( "\twaiting for ack...\n" );
	MRIReconAck ack;
	if( !communicator.ReceiveReconAck( ack ) )
		mexErrMsgTxt( "Error receiving ACK!\n" );

	mexPrintf( "%s\n", ack.ToString().c_str() );

	// get data
	MRIData data_back;
	if( ack.success )
	{
		mexPrintf( "waiting for data back...\n" );
		if( !communicator.ReceiveData( data_back ) )
			mexErrMsgTxt( "error receiving data...\n" );
		// send response ack
		MRIReconAck re_ack;
		re_ack.success = true;
		communicator.SendReconAck( re_ack );
	}
	else
		mexPrintf( "Ack failed!\n" );

	// close connection
	plhs[0] = MexData::ExportMexArray( data_back );
	communicator.CloseConnection();
	mexPrintf( "Done!\n" );
}
