#ifndef PLUGIN_PIPES_H
#define PLUGIN_PIPES_H

#include <ReconPlugin.h>
#include <string>

class Plugin_FileTransfer: public ReconPlugin
{
	public:
	Plugin_FileTransfer( const char* new_plugin_id, const char* new_alias ): ReconPlugin( new_plugin_id, new_alias ) {}

	protected:
	std::string ext_prog_dir;
	std::string ext_prog;
	GIRConfig accum_config;

	bool Reconstruct( MRIData& mri_data );
	bool Configure( GIRConfig& config, bool main_config, bool final_config );
};

#endif
