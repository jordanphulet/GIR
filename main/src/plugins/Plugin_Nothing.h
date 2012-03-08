#ifndef PLUGIN_NOTHING_H
#define PLUGIN_NOTHING_H

#include <ReconPlugin.h>

class Plugin_Nothing: public ReconPlugin
{
	public:
	Plugin_Nothing( const char* new_plugin_id, const char* new_alias ): ReconPlugin( new_plugin_id, new_alias ) {}

	protected:
	bool Reconstruct( MRIData& mri_data );
	bool Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data );
	bool Configure( GIRConfig& config, bool main_config, bool final_config );
	virtual bool CanReconMeasData() { return false; }
};

#endif
