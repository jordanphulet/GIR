#ifndef PLUGIN_RADIAL_GRIDDER_H
#define PLUGIN_RADIAL_GRIDDER_H

#include <ReconPlugin.h>
#include <Serializable.h>
#include <RadialGridder.h>

class GIRConfig;

class Plugin_RadialGridder: public ReconPlugin
{
	public:
	Plugin_RadialGridder( const char* new_plugin_id, const char* new_alias ): ReconPlugin( new_plugin_id, new_alias ), flatten( true ) {}

	protected:
	RadialGridder::ViewOrderingType view_ordering;
	bool flatten;

	bool Configure( GIRConfig& config, bool main_config, bool final_config );
	bool Reconstruct( MRIData& mri_data );
	bool Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data );
};

#endif
