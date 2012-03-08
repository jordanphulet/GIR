#include <MRIDataSplitter.h>
#include <MRIData.h>
#include <GIRLogger.h>

bool MRIDataSplitter::SplitRepetitions( MRIData& full_data, MRIData& sub_data, int full_start_rep, int num_reps )
{
	//GIRLogger::LogInfo( "### splitting: start: %d, num: %d\n", full_start_rep, num_reps );
	// initialize new data
	MRIDimensions new_dims = full_data.Size();
	new_dims.Repetition = num_reps;
	sub_data = MRIData( new_dims, full_data.IsComplex() );

	return CopyRepetitions( full_data, sub_data, full_start_rep, 0, num_reps, false );
}

bool MRIDataSplitter::MergeRepetitions( MRIData& full_data, MRIData& sub_data, int full_start_rep, int sub_start_rep, int num_reps )
{
	return CopyRepetitions( full_data, sub_data, full_start_rep, sub_start_rep, num_reps, true );
}


bool MRIDataSplitter::CopyRepetitions( MRIData& full_data, MRIData& sub_data, int full_start_rep, int sub_start_rep, int num_reps, bool sub_to_full )
{
	if( num_reps < 1 )
	{
		GIRLogger::LogError( "MRIDataSplitter::CopyRepetitions -> invalid num_reps: %d, aborting!\n", num_reps );
		return false;
	}
	if( full_start_rep < 0 || full_start_rep >= full_data.Size().Repetition || ( full_start_rep + num_reps ) > full_data.Size().Repetition )
	{
		GIRLogger::LogError( "MRIDataSplitter::CopyRepetitions -> out of bounds, full_start_rep: %d, num_reps: %d, aborting!\n", full_start_rep, num_reps );
		return false;
	}
	if( sub_start_rep < 0 || sub_start_rep >= sub_data.Size().Repetition || ( sub_start_rep + num_reps ) > sub_data.Size().Repetition )
	{
		GIRLogger::LogError( "MRIDataSplitter::CopyRepetitions -> out of bounds, sub_start_rep: %d, num_reps: %d, aborting!\n", sub_start_rep, num_reps );
		return false;
	}
	if( sub_data.IsComplex() != full_data.IsComplex() )
	{
		GIRLogger::LogError( "MRIDataSplitter::CopyRepetitions -> complexity must match, aborting!\n" );
		return false;
	}

	MRIDimensions full_dim_check = full_data.Size();
	full_dim_check.Repetition = 1;
	MRIDimensions sub_dim_check = sub_data.Size();
	sub_dim_check.Repetition = 1;
	if( !full_dim_check.Equals( sub_dim_check ) )
	{
		GIRLogger::LogError( "MRIDataSplitter::CopyRepetitions -> incompatible dimensions, aborting!\n" );
		return false;
	}

	// copy data, this could be faster
	int copy_size = full_data.Size().Column * sizeof( float );
	if( full_data.IsComplex() )
		copy_size *= 2;
	MRIDimensions sub_dims = sub_data.Size();
	for( int line = 0; line < sub_dims.Line; line++ )
	for( int channel = 0; channel < sub_dims.Channel; channel++ )
	for( int set = 0; set < sub_dims.Set; set++ )
	for( int phase = 0; phase < sub_dims.Phase; phase++ )
	for( int slice = 0; slice < sub_dims.Slice; slice++ )
	for( int echo = 0; echo < sub_dims.Echo; echo++ )
	for( int repetition = 0; repetition < num_reps; repetition++ )
	for( int segment = 0; segment < sub_dims.Segment; segment++ )
	for( int partition = 0; partition < sub_dims.Partition; partition++ )
	for( int average = 0; average < sub_dims.Average; average++ )
	{
		float* full_index = full_data.GetDataIndex( 0, line, channel, set, phase, slice, echo, repetition+full_start_rep, segment, partition, average );
		float* sub_index = sub_data.GetDataIndex( 0, line, channel, set, phase, slice, echo, repetition+sub_start_rep, segment, partition, average );
		if( sub_to_full )
			memcpy( full_index, sub_index, copy_size );
		else
			memcpy( sub_index, full_index, copy_size );
	}

	return true;
}
