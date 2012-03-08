#include <string>
#include <mex.h> 
#include <MRIData.h>
#include <matlab/MexData.h>
#include <TCPCommunicator.h>
//#include <Serializable.h>
#include <MRIDataComm.h>
#include <SiemensTool.h>
#include <GIRLogger.h>
#include <stdio.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
	char* dat_file_path = mxArrayToString( prhs[0] );

	if( dat_file_path == 0 )
	{
		mexPrintf( "dat_file_path needs to be a string.\n" );
		return;
	}

	MRIDataHeader header;
	std::vector<MRIMeasurement> meas_vector;
	SiemensTool::LoadDatFile( dat_file_path, header, meas_vector );
		
	MRIData mri_data( header.Size(), true );

	std::vector<MRIMeasurement>::iterator it;
	for( it = meas_vector.begin(); it != meas_vector.end(); it++ )
		if( !it->UnloadData( mri_data ) )
			GIRLogger::LogError( "DataSorter::SortReplace -> MRIMasurement::UnloadData failed, measurement was not added!\n" );

	mexPrintf( "header size: %s\n", header.Size().ToString().c_str() );

	plhs[0] = MexData::ExportMexArray( mri_data );
}
