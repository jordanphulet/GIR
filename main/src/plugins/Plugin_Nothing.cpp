#include <GIRConfig.h>
#include <GIRLogger.h>
#include <plugins/Plugin_Nothing.h>

extern "C" ReconPlugin* create( const char* alias )
{
	return new Plugin_Nothing( "Plugin_Nothing", alias );
}

extern "C" void destroy( ReconPlugin* plugin )
{
	delete plugin;
}

bool Plugin_Nothing::Configure( GIRConfig& config, bool main_config, bool final_config )
{
	return true;
}

bool Plugin_Nothing::Reconstruct( MRIData& mri_data )
{
	return true;
}

bool Plugin_Nothing::Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data )
{
	GIRLogger::LogError( "Plugin_Nothing::Reconstruct -> reconstruction with MRIMeasurement vector not implemented!\n" );
	return false;
}
