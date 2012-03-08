#include <MRIDataTool.h>
#include <FilterTool.h>
#include <Serializable.h>
#include <GIRLogger.h>
#include <GIRConfig.h>
#include <Plugin_TCR.h>
#include <TCRIteratorCPU.h>
#ifndef NO_CUDA
	#include <TCRIteratorCUDA.h>
#endif

extern "C" ReconPlugin* create( const char* alias )
{
	return new Plugin_TCR( "Plugin_TCR", alias );
}

extern "C" void destroy( ReconPlugin* plugin )
{
	delete plugin;
}

bool Plugin_TCR::Configure( GIRConfig& config, bool main_config, bool final_config )
{
	bool success = true;
	config.GetParam( plugin_id.c_str(), alias.c_str(), "alpha", alpha );
	config.GetParam( plugin_id.c_str(), alias.c_str(), "beta", beta );
	config.GetParam( plugin_id.c_str(), alias.c_str(), "beta_squared", beta_squared );
	config.GetParam( plugin_id.c_str(), alias.c_str(), "step_size", step_size );
	config.GetParam( plugin_id.c_str(), alias.c_str(), "iterations", iterations );
	config.GetParam( plugin_id.c_str(), alias.c_str(), "threads", threads);
	config.GetParam( plugin_id.c_str(), alias.c_str(), "use_gpu", use_gpu );
	config.GetParam( plugin_id.c_str(), alias.c_str(), "gpu_thread_load", gpu_thread_load );

	std::string temp_dim_string;
	if( config.GetParam( plugin_id.c_str(), alias.c_str(), "temporal_dimension", temp_dim_string ) )
	{
		if( temp_dim_string.compare( "phase" ) == 0 )
			temp_dim = TCRIterator::TEMP_DIM_PHASE;
		else if( temp_dim_string.compare( "repetition" ) == 0 )
			temp_dim = TCRIterator::TEMP_DIM_REP;
		else
			GIRLogger::LogError( "Plugin_TCR::Configure -> invalid temporal_dimension: \"%s\"!\n", temp_dim_string.c_str() );
	}

	if( alpha < 0 )
	{
		GIRLogger::LogError( "Plugin_TCR::Configure -> alpha cannot be less than 0 as specified!\n" );
		success = false;
	}

	if( beta < 0 )
	{
		GIRLogger::LogError( "Plugin_TCR::Configure -> beta cannot be less than 0 as specified!\n" );
		success = false;
	}

	if( step_size < 0 )
	{
		GIRLogger::LogError( "Plugin_TCR::Configure -> step_size cannot be less than 0 as specified!\n" );
		success = false;
	}

	if( iterations < 0 )
	{
		GIRLogger::LogError( "Plugin_TCR::Configure -> iterations cannot be less than 0 as specified!\n" );
		success = false;
	}

	if( threads < 1 )
	{
		GIRLogger::LogError( "Plugin_TCR::Configure -> threads cannot be less than 1 as specified!\n" );
		success = false;
	}

	if( gpu_thread_load < 1 )
	{
		GIRLogger::LogError( "Plugin_TCR::Configure -> gpu_thread_load cannot be less than 1 as specified!\n" );
		success = false;
	}

	return success;
}

bool Plugin_TCR::Reconstruct( MRIData& mri_data )
{
	// scale data
	mri_data.ScaleMax( 150 );

	// shift to corner
	FilterTool::FFTShift( mri_data );

	// generate original estimate from interpolated k-space to get rid of blurring
	GIRLogger::LogInfo( "Plugin_TCR::Reconstruct -> generating original estimate...\n" );
	MRIData estimate;
	MRIDataTool::TemporallyInterpolateKSpace( mri_data, estimate );
	FilterTool::FFT2D( estimate, true );
	
	// generate coil map
	GIRLogger::LogInfo( "Plugin_TCR::Reconstruct -> generating coil map...\n" );
	MRIData coil_map;
	MRIDataTool::GetCoilSense( estimate, coil_map );

	// generate lambda map
	MRIData lambda_map;
	try
	{
		MRIDataTool::GetPhaseContrastLambdaMap( estimate, lambda_map );
	}
	catch( ... )
	{
		GIRLogger::LogError( "Plugin_TCR::Reconstruct -> lambda map generation failed, FIX THIS!\n" );
		MRIDimensions lambda_dims( estimate.Size().Column, estimate.Size().Line, 1, 1, 1, 1, 1, 1, 1, 1, 1 );
		lambda_map = MRIData( lambda_dims, false );
		lambda_map.SetAll( 1 );
	}

	// iterate
	if( use_gpu )
	{
#ifndef NO_CUDA
		GIRLogger::LogInfo( "Plugin_TCR::Reconstruct -> reconstructing on GPU...\n" );
		TCRIteratorCUDA iterator( gpu_thread_load, temp_dim );
		iterator.Load( alpha, beta, beta_squared, step_size, mri_data, estimate, coil_map, lambda_map );
		iterator.Iterate( iterations );
		iterator.Unload( mri_data );
#else
		GIRLogger::LogError( "Plugin_TCR::Reconstruct -> GPU requested but binaries not build with CUDA, aborting!\n" );
		return false;
#endif
	}
	else
	{
		GIRLogger::LogInfo( "Plugin_TCR::Reconstruct -> reconstructing on CPU(s)...\n" );
		TCRIteratorCPU iterator( threads, temp_dim );
		iterator.Load( alpha, beta, beta_squared, step_size, mri_data, estimate, coil_map, lambda_map );
		iterator.Iterate( iterations );
		iterator.Unload( mri_data );
	}

	// shift to center
	FilterTool::FFTShift( mri_data, true );
	GIRLogger::LogDebug( "### ALL done iterating\n" );

	return true;
}

bool Plugin_TCR::Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data )
{
	GIRLogger::LogError( "Plugin_TCR::Reconstruct -> reconstruction with MRIMeasurement vector not implemented!\n" );
	return false;
}
