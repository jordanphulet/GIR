#include <MPIPartitioner.h>
#include <MRIData.h>
#include <MRIDataSplitter.h>
#include <GIRLogger.h>
#include <math.h>

bool MPIPartitioner::SplitRepetitions( MRIData& full_data, MRIData& sub_data, int rank, int tasks, int overlap )
{
	int split_start;
	int split_end;
	SplitRepetitions( full_data, sub_data, rank, tasks, overlap, split_start, split_end );
}

bool MPIPartitioner::SplitRepetitions( MRIData& full_data, MRIData& sub_data, int rank, int tasks, int overlap, int& split_start, int& split_end )
{
	if( rank >= tasks )
	{
		GIRLogger::LogError( "MPIPartitioner::SplitRepetitions -> rank must be less than tasks!\n" );
		return false;
	}

	float reps_per_task = full_data.Size().Repetition / (float)tasks;

	int body_start = (int)ceil( reps_per_task * rank );
	int body_end = (int)ceil( reps_per_task * (rank+1) );

	split_start = std::max( body_start - overlap, 0 );
	split_end = std::min( body_end + overlap, full_data.Size().Repetition );
	int split_size = split_end - split_start;

	GIRLogger::LogDebug( "### SPLITTING -> reps_per_task: %f, split_start: %d, split_end: %d, split_size: %d...\n", reps_per_task, split_start, split_end, split_size );
	GIRLogger::LogDebug( "\trank: %d, tasks: %d, overlap: %d...\n", rank, tasks, overlap );

	return MRIDataSplitter::SplitRepetitions( full_data, sub_data, split_start, split_size );
}

bool MPIPartitioner::MergeRepetitions( MRIData& full_data, MRIData& sub_data, int rank, int tasks, int overlap )
{

	float reps_per_task = full_data.Size().Repetition / (float)tasks;

	int merge_start = (int)ceil( reps_per_task * rank );
	int merge_end = std::min( (int)ceil( reps_per_task * (rank+1) ), full_data.Size().Repetition );;
	int merge_size = merge_end - merge_start;

	int sub_start_rep = overlap;
	if( merge_start < overlap )
		sub_start_rep += merge_start - overlap;

	GIRLogger::LogDebug( "### MERGING -> reps_per_task: %f, merge_start: %d, merge_end: %d, merge_size: %d, sub_start_rep: %d...\n", reps_per_task, merge_start, merge_end, merge_size, sub_start_rep );
	GIRLogger::LogDebug( "###      full: %s\n", full_data.Size().ToString().c_str() );
	GIRLogger::LogDebug( "###      sub: %s\n", sub_data.Size().ToString().c_str() );

	return MRIDataSplitter::MergeRepetitions( full_data, sub_data, merge_start, sub_start_rep, merge_size );
}
