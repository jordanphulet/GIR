#include <GIRConfig.h>
#include <GIRLogger.h>
#include <plugins/Plugin_SortCombine.h>

extern "C" ReconPlugin* create( const char* alias )
{
	return new Plugin_SortCombine( "Plugin_SortCombine", alias );
}

extern "C" void destroy( ReconPlugin* plugin )
{
	delete plugin;
}

bool Plugin_SortCombine::Configure( GIRConfig& config, bool main_config, bool final_config )
{
	return true;
}

bool Plugin_SortCombine::Reconstruct( MRIData& mri_data )
{
	return true;
}

bool Plugin_SortCombine::Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data )
{
	std::vector<MRIMeasurement>::iterator it;
	for( it = meas_vector.begin(); it != meas_vector.end(); it++ )
		if( !it->UnloadData( mri_data ) )
			GIRLogger::LogError( "Plugin_SortCombine::Reconstruct-> MRIMasurement::UnloadData failed, measurement was not added!\n" );
	return true;
}
