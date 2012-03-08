#ifndef PLUGIN_PIPES_H
#define PLUGIN_PIPES_H

#include <ReconPlugin.h>
#include <string>

class Plugin_Pipes: public ReconPlugin
{
	public:
	Plugin_Pipes( const char* new_plugin_id, const char* new_alias ): ReconPlugin( new_plugin_id, new_alias ) {}

	protected:
	std::string pipe_prog_dir;
	std::string pipe_prog;

	bool Reconstruct( MRIData& mri_data );
	bool Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data );
	bool Configure( GIRConfig& config, bool main_config, bool final_config );
};

#endif
