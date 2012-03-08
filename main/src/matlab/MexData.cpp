#include "mex.h"
#include <matlab/MexData.h>
#include <MRIData.h>
#include <GIRLogger.h>
#include <cstdio>
#include <cstring>

bool MexData::ImportMexArray( MRIData& mri_data, const mxArray* mx_array, bool allow_resize )
{
	if( !mxIsSingle( mx_array ) )
	{
		fprintf( stderr, "MexData::ImportMexArray -> data must be single precision!\n" );
		return false;
	}

	// get dimensions of array
	const mwSize *array_size = mxGetDimensions( mx_array );
	mwSize array_dims = mxGetNumberOfDimensions( mx_array );

	// can't have more than 11 dimensions
	if( array_dims > 11 )
	{
		fprintf( stderr, "MexData::ImportMexArray -> when attaching a mex array it can not have more than 11 dimensions (column,line,channel,set,phase,slice)!\n" );
		return false;
	}

	int num_columns =		( array_dims > 0 ) ? array_size[0] : 1;
	int num_lines =			( array_dims > 1 ) ? array_size[1] : 1;
	int num_channels =		( array_dims > 2 ) ? array_size[2] : 1;
	int num_sets =			( array_dims > 3 ) ? array_size[3] : 1;
	int num_phases =		( array_dims > 4 ) ? array_size[4] : 1;
	int num_slices =		( array_dims > 5 ) ? array_size[5] : 1;
	int num_echos =			( array_dims > 6 ) ? array_size[6] : 1;
	int num_repetitions =	( array_dims > 7 ) ? array_size[7] : 1;
	int num_segments =		( array_dims > 8 ) ? array_size[8] : 1;
	int num_partitions =	( array_dims > 9 ) ? array_size[9] : 1;
	int num_averages =		( array_dims > 10 ) ? array_size[10] : 1;

	const MRIDimensions& size = mri_data.Size();
	if( allow_resize )
	{
		MRIDimensions new_dims( num_columns, num_lines, num_channels, num_sets, num_phases, num_slices, num_echos, num_repetitions, num_partitions, num_segments, num_averages );
		mri_data = MRIData( new_dims, mxIsComplex( mx_array ) );
	}
	else if
	( 
		num_columns != size.Column ||
		num_lines != size.Line ||
		num_channels != size.Channel ||
		num_sets != size.Set ||
		num_phases != size.Phase ||
		num_slices != size.Slice ||
		num_echos != size.Echo ||
		num_repetitions != size.Repetition ||
		num_partitions != size.Partition ||
		num_segments != size.Segment ||
		num_averages != size.Average ||
		mri_data.IsComplex() != mxIsComplex( mx_array ) 
	)
	{

		const char *this_complexity = ( mri_data.IsComplex() )? "complex" : "real";
		const char *that_complexity = ( mxIsComplex( mx_array ) )? "complex" : "real";

		fprintf
		( 
			stderr, 
			"MexData::ImportMexArray -> allow_resize is FALSE and the mxArray dimensions don't match the MRIData!\n"
			"\tcomplexity=%s<->%s columns=%d<->%d lines=%d<->%d channels=%d<->%d sets=%d<->%d phases=%d<->%d slices=%d<->%d echos=%d<->%d repetitions=%d<->%d partitions=%d<->%d segments=%d<->%d averages=%d<->%d\n",
			this_complexity, that_complexity,
			size.Column, num_columns,
			size.Line, num_lines,
			size.Channel, num_channels,
			size.Set, num_sets,
			size.Phase, num_phases,
			size.Slice, num_slices,
			size.Echo, num_echos,
			size.Repetition, num_repetitions,
			size.Partition, num_partitions,
			size.Segment, num_segments,
			size.Average, num_averages
		);
		return false;
	}

	float* start_address = mri_data.GetDataStart();
	// for real data  just memcpy
	if( !mri_data.IsComplex() )
		memcpy( start_address, mxGetPr( mx_array ), mri_data.NumElements() * sizeof( float ) );
	// complex data needs to be interleaved
	else
	{
		float* ref_real = (float*)mxGetPr( mx_array );
		float* ref_imaginary = (float*)mxGetPi( mx_array );
		for( int i = 0; i < mri_data.NumPixels(); i++ ) {
			start_address[2*i] = ref_real[i];
			start_address[2*i+1] = ref_imaginary[i];
		}
	}
	return true;
}

mxArray* MexData::ExportMexArray( MRIData& mri_data ) {
	// create array
	const MRIDimensions size = mri_data.Size();
	mwSize dims[] = { size.Column, size.Line, size.Channel, size.Set, size.Phase, size.Slice, size.Echo, size.Repetition, size.Segment, size.Partition, size.Average };

	//GIRLogger::LogDebug( "###col: %d, line: %d, channel: %d, set: %d, phase: %d, slice: %d, echo: %d, rep: %d, seg: %d, part:%d, ave: %d\n", size.Column, size.Line, size.Channel, size.Set, size.Phase, size.Slice, size.Echo, size.Repetition, size.Segment, size.Partition, size.Average );
	//GIRLogger::LogDebug( "### num elements: %d\n", mri_data.NumElements() );

	mxComplexity complexity = ( mri_data.IsComplex() )? mxCOMPLEX : mxREAL;
	mxArray* new_array = mxCreateNumericArray( 11, dims, mxSINGLE_CLASS, complexity );

	float* start_address = mri_data.GetDataStart();
	// real: just memcpy
	if( !mri_data.IsComplex() )
		memcpy( mxGetPr( new_array ), start_address,  mri_data.NumElements() * sizeof( float ) );
	// complex: need to un-interleave
	else {
		float* ref_real = (float*)mxGetPr( new_array );
		float* ref_imaginary = (float*)mxGetPi( new_array );
		for( int i = 0; i < mri_data.NumElements()/2; i++ ) {
			ref_real[i] = start_address[2*i];
			ref_imaginary[i] = start_address[2*i+1];
		}
	}
	return new_array;
}
