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
	mexPrintf( "    mri_data = UnserializeData( ser_file_path )\n" );
}

bool CheckArgs( int nlhs, int nrhs, const mxArray* prhs[] )
{
	// check for correct number of arguments
	if( nrhs != 1 || nlhs != 1 )
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

	char* ser_file_path = mxArrayToString( prhs[0] );

	// create file communicator
	FileCommunicator communicator;
	if( !communicator.OpenInput( ser_file_path ) )
		mexErrMsgTxt( "unable to open input!\n" );

	// get request
	MRIReconRequest request;
	communicator.ReceiveReconRequest( request );

	// get data
	MRIData mri_data;
	communicator.ReceiveData( mri_data );

	// export k-space
	plhs[0] = MexData::ExportMexArray( mri_data );
}
