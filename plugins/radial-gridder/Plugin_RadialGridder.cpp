#include <MRIDataTool.h>
#include <Serializable.h>
#include <GIRLogger.h>
#include <GIRConfig.h>
#include <Plugin_RadialGridder.h>
#include <RadialGridder.h>

extern "C" ReconPlugin* create( const char* alias )
{
	return new Plugin_RadialGridder( "Plugin_RadialGridder", alias );
}

extern "C" void destroy( ReconPlugin* plugin )
{
	delete plugin;
}

bool Plugin_RadialGridder::Configure( GIRConfig& config, bool main_config, bool final_config )
{
	// get view_ordering param
	std::string view_ordering_string = "NONE";
	if( config.GetParam( plugin_id.c_str(), alias.c_str(), "view_ordering", view_ordering_string ) )
	{
		if( view_ordering_string.compare( "NONE" ) == 0 )
			view_ordering = RadialGridder::VO_NONE;
		else if( view_ordering_string.compare( "JORDAN_JITTER" ) == 0 )
			view_ordering = RadialGridder::VO_JORDAN_JITTER;
		else if( view_ordering_string.compare( "GOLDEN_RATIO" ) == 0 )
			view_ordering = RadialGridder::VO_GOLDEN_RATIO;
		else if( view_ordering_string.compare( "GOLDEN_RATIO_NO_PHASE" ) == 0 )
			view_ordering = RadialGridder::VO_GOLDEN_RATIO_NO_PHASE;
		else
		{
			GIRLogger::LogError( "Plugin_RadialGridder::Configure -> invalid view_ordering: '%s'!\n", view_ordering_string.c_str() );
			return false;
		}
	}

	// get flatten
	config.GetParam( plugin_id.c_str(), alias.c_str(), "flatten", flatten );

	return true;
}

bool Plugin_RadialGridder::Reconstruct( MRIData& mri_data )
{
	MRIData gridded;
	RadialGridder gridder;
	gridder.view_ordering = view_ordering;

	GIRLogger::LogInfo( "Plugin_RadialGridder::Reconstruct -> gridding...\n" );
	if( !flatten )
		GIRLogger::LogInfo( "Plugin_RadialGridder::Reconstruct -> not flattening...\n" );

	if( gridder.Grid( mri_data, gridded, RadialGridder::KERN_TYPE_BILINEAR, 101, flatten) )
	{
		mri_data = gridded;
		return true;
	}
	else
		return false;
}

bool Plugin_RadialGridder::Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data )
{
	GIRLogger::LogError( "Plugin_RadialGridder::Reconstruct -> reconstruction with MRIMeasurement vector not implemented!\n" );
	return false;
}
