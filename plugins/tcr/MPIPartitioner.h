#ifndef MPI_PARTITIONER_H
#define MPI_PARTITIONER_H
class MRIData;

class MPIPartitioner
{
	public:
	static bool SplitRepetitions( MRIData& full_data, MRIData& sub_data, int rank, int tasks, int overlap );
	static bool SplitRepetitions( MRIData& full_data, MRIData& sub_data, int rank, int tasks, int overlap, int& split_start, int& split_end );
	static bool MergeRepetitions( MRIData& full_data, MRIData& sub_data, int rank, int tasks, int overlap );
};

#endif
