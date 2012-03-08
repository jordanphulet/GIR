#include <MRIData.h>
#include <FileCommunicator.h>
//#include <Serializable.h>
#include <MRIDataComm.h>
#include <SiemensTool.h>
#include <matlab/MexData.h>
#include <string>
#include <mex.h> 
#include <stdio.h>
#include <vector>

void PrintUsage() {
	mexPrintf( "Usage:\n" );
	mexPrintf( "    SerializeData( mri_data, ser_file_path )\n" );
}

bool CheckArgs( int nlhs, int nrhs, const mxArray* prhs[] )
{
	// check for correct number of arguments
	if( nrhs != 2 )
	{
		mexPrintf( "Wrong number of arguments!\n" );
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

	char* ser_file_path = mxArrayToString( prhs[1] );

	// import k-space
	MRIData mri_data;
	if( !MexData::ImportMexArray( mri_data, prhs[0], true ) )
		mexErrMsgTxt( "unable to import mxArray!\n" );
	
	// create file communicator
	FileCommunicator communicator;
	communicator.OpenOutput( ser_file_path );

	// send request
	MRIReconRequest request;
	request.pipeline = "nothing";
	communicator.SendReconRequest( request );

	// send data
	communicator.SendData( mri_data );
}
