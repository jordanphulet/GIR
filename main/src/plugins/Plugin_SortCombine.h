#ifndef PLUGIN_SORT_COMBINE_H
#define PLUGIN_SORT_COMBINE_H

#include <ReconPlugin.h>

class Plugin_SortCombine: public ReconPlugin
{
	public:
	Plugin_SortCombine( const char* new_plugin_id, const char* new_alias ): ReconPlugin( new_plugin_id, new_alias ) {}

	protected:
	bool Reconstruct( MRIData& mri_data );
	bool Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data );
	bool Configure( GIRConfig& config, bool main_config, bool final_config );
	virtual bool CanReconMeasData() { return true; }
};

#endif
