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
	mexPrintf( "    mri_data = RecvDat( port )\n" );
}

bool CheckArgs( int nlhs, int nrhs, const mxArray* prhs[] )
{
	// check for correct number of arguments
	if( nrhs != 1 && nrhs != 1 )
	{
		mexPrintf( "wrong number of arguments!\n" );
		return false;
	}

	// make sure port wasn't specified as a string
	if( mxArrayToString( prhs[0] ) != 0 )
	{
		mexPrintf( "port cannot be a string!\n" );
		return false;
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

	int port = (int)mxGetScalar( prhs[0] );

	// connect
	TCPCommunicator communicator;
	if( !communicator.Listen( port ) )
		mexErrMsgTxt( "communicator.Listen() failed!\n" );

	mexPrintf( "waiting for connection...\n" );
	if( communicator.AcceptConnection() )
	{
		mexPrintf( "client connected...\n" );

		// get recon request
		mexPrintf( "\tgetting request...\n" );
		MRIReconRequest request;
		if( !communicator.ReceiveReconRequest( request ) )
			mexErrMsgTxt( "communicator.ReceiveReconRequest failed!\n" );
			
		// get data
		MRIData mri_data;
		if( !communicator.ReceiveData( mri_data ) )
			mexErrMsgTxt( "communicator.ReceiveData failed!\n" );
		plhs[0] = MexData::ExportMexArray( mri_data );
		
	}
	else
	{
		mexErrMsgTxt( "communicator.AcceptConnection failed!\n" );
	}

	// close connection
	communicator.CloseConnection();
	mexPrintf( "Done!\n" );
}
