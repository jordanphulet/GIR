#include <MRIDataTool.h>
#include <Serializable.h>
#include <GIRLogger.h>
#include <GIRConfig.h>
#include <Plugin_TempInterpK.h>

extern "C" ReconPlugin* create( const char* alias )
{
	return new Plugin_TempInterpK( "Plugin_TempInterpK", alias );
}

extern "C" void destroy( ReconPlugin* plugin )
{
	delete plugin;
}

bool Plugin_TempInterpK::Configure( GIRConfig& config, bool main_config, bool final_config )
{
	return true;
}

bool Plugin_TempInterpK::Reconstruct( MRIData& mri_data )
{
	GIRLogger::LogInfo( "Plugin_TempInterpK:Reconstruct -> really interpolating...\n" );
	MRIData temp_data = mri_data;
	if( !MRIDataTool::TemporallyInterpolateKSpace( temp_data, mri_data ) )
	{
		GIRLogger::LogError( "Plugin_TempInterpK::Reconstruct -> MRIDataTool::TemporallyInterpolateKSpace failed, aborting!\n" );
		return false;
	}
	return true;
}

bool Plugin_TempInterpK::Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data )
{
	GIRLogger::LogError( "Plugin_TempInterpK::Reconstruct -> reconstruction with MRIMeasurement vector not implemented!\n" );
	return false;
}
