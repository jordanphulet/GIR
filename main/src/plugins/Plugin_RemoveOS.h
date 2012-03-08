#ifndef PLUGIN_REMOVE_OS_H
#define PLUGIN_REMOVE_OS_H

#include <ReconPlugin.h>
#include <Serializable.h>

class GIRConfig;

class Plugin_RemoveOS: public ReconPlugin
{
	public:
	Plugin_RemoveOS( const char* new_plugin_id, const char* new_alias ): os_x( 1 ), os_y( 1 ), ReconPlugin( new_plugin_id, new_alias ) {}

	protected:
	float os_x;
	float os_y;

	void LoadParams();
	bool Reconstruct( MRIData& mri_data );
	bool Configure( GIRConfig& config, bool main_config, bool final_config );
};

#endif
