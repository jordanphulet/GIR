#ifndef PLUGIN_TCR_H
#define PLUGIN_TCR_H

#include <ReconPlugin.h>
#include <Serializable.h>
#include <TCRIterator.h>

class GIRConfig;

class Plugin_TCR: public ReconPlugin
{
	public:
	Plugin_TCR( const char* new_plugin_id, const char* new_alias ): ReconPlugin( new_plugin_id, new_alias ), alpha( 1 ), beta( 1 ), beta_squared( 0.00001 ), step_size( 1 ), iterations( 10 ), threads( 1 ), use_gpu( false ), gpu_thread_load( 1 ), temp_dim( TCRIterator::TEMP_DIM_PHASE ) {}

	protected:
	float alpha;
	float beta;
	float beta_squared;
	float step_size;
	int iterations;
	int threads;
	bool use_gpu;
	int gpu_thread_load;
	TCRIterator::TemporalDimension temp_dim;

	bool Configure( GIRConfig& config, bool main_config, bool final_config );
	bool Reconstruct( MRIData& mri_data );
	bool Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data );
};

#endif
