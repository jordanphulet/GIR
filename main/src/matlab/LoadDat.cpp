#include <MRIData.h>
#include <TCPCommunicator.h>
#include <MRIDataComm.h>
#include <SiemensTool.h>
#include <matlab/MexData.h>
#include <string>
#include <mex.h> 
#include <stdio.h>
#include <vector>

void PrintUsage() {
	mexPrintf( "Usage:\n" );
	mexPrintf( "    k_space = LoadDat( dat_file_path )\n" );
	mexPrintf( "    [k_space meas_data] = LoadDat( dat_file_path )\n" );
}


bool CheckArgs( int nlhs, int nrhs, const mxArray* prhs[] )
{
	// check for correct number of arguments
	if( nrhs != 1 || ( nlhs != 1 && nlhs != 2 ) )
	{
		mexPrintf( "wrong number of arguments!\n" );
		return false;
	}
	return true;
}

void CreateMeasStructure( std::vector<MRIMeasurement>& meas_vector, mxArray** meas_struct )
{
	// create structure
	const char* field_names [] = { 
		"measurement",
		"indicies"
	};
	mwSize struct_dims = meas_vector.size();
	*meas_struct = mxCreateStructArray( 1, &struct_dims, 2, field_names );

	// fill structure
	int num_dims = MRIDimensions::GetNumDims();
	for( unsigned int i = 0; i < meas_vector.size(); i++ )
	{
		// fill measurement mxArray
		mxSetFieldByNumber( *meas_struct, i, 0, MexData::ExportMexArray( meas_vector[i].GetData() ) );

		// create index mxArray
		//MRIDimensions data_size = meas_vector[i].GetData().Size();
		MRIDimensions data_size = meas_vector[i].GetIndex();
		mwSize dims = num_dims + 1; // we need all the dim indicies plus meas time
		mxArray* meas_array = mxCreateNumericArray( 1, &dims, mxSINGLE_CLASS, mxREAL );
		float* index_data = (float*)mxGetPr( meas_array );
		int dim_size;
		for( int j = 0; j < num_dims; j++ )
		{
			data_size.GetDim( j, dim_size );
			// 1 based matlab matrix indicies...
			index_data[j] = dim_size+1;
		}
		index_data[num_dims] = meas_vector[i].meas_time;
		mxSetFieldByNumber( *meas_struct, i, 1, meas_array );
	}
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

	// make sure arguments are valid
	if( !CheckArgs( nlhs, nrhs, prhs ) )
	{
		PrintUsage();
		return;
	}

	char* dat_file_path = mxArrayToString( prhs[0] );

	if( dat_file_path == 0 )
	{
		mexPrintf( "dat_file_path needs to be strings!\n" );
		return;
	}

	// load data
	mexPrintf( "loading %s...\n", dat_file_path );
	MRIDataHeader header;
	std::vector<MRIMeasurement> meas_vector;
	if( !SiemensTool::LoadDatFile( dat_file_path, header, meas_vector ) )
		mexErrMsgTxt( "Loading dat file failed!\n" );

	// load measurements
	MRIData mri_data( header.Size(), header.IsComplex() );
	std::vector<MRIMeasurement>::iterator it;
	for( it = meas_vector.begin(); it != meas_vector.end(); it++ )
		if( !it->UnloadData( mri_data ) )
			mexErrMsgTxt( "MRIMasurement::UnloadData failed!\n" );
		
	// export
	plhs[0] = MexData::ExportMexArray( mri_data );

	if( nlhs > 1 )
	{
		mxArray* meas_struct = 0;
		CreateMeasStructure( meas_vector, &meas_struct );
		plhs[1] = meas_struct;
	}

	mexPrintf( "Done!\n" );
}
