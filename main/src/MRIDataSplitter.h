#ifndef MRI_DATA_SPLITTER
#define MRI_DATA_SPLITTER

class MRIData;

class MRIDataSplitter
{
	public:
	static bool SplitRepetitions( MRIData& full_data, MRIData& sub_data, int full_start_rep, int num_reps );
	static bool MergeRepetitions( MRIData& full_data, MRIData& sub_data, int full_start_rep, int sub_start_rep, int num_reps );

	private:
	static bool CopyRepetitions( MRIData& full_data, MRIData& sub_data, int full_start_rep, int sub_start_rep, int num_reps, bool sub_to_full );
};

#endif
