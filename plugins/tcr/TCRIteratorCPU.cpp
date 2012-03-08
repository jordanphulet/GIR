#include <TCRIteratorCPU.h>
#include <GIRLogger.h>
#include <MRIData.h>
#include <MRIDataTool.h>
#include <FilterTool.h>
#include <KernelCode.h>
#include <vector>
#include <pthread.h>
#include <cmath>

TCRIteratorCPU::~TCRIteratorCPU()
{
	if( meas_data != 0 ) { delete [] meas_data; meas_data = 0; }
	if( estimate != 0 ) { delete [] estimate; estimate = 0; }
	if( gradient != 0 ) { delete [] gradient; gradient = 0; }
	if( coil_map != 0 ) { delete [] coil_map; coil_map = 0; }
	if( lambda_map != 0 ) { delete [] lambda_map; lambda_map = 0; }
}

void TCRIteratorCPU::Load( float alpha, float beta, float beta_squared, float step_size, MRIData& src_meas_data, MRIData& src_estimate, MRIData& src_coil_map, MRIData& src_lambda_map )
{
	// call parent
	TCRIterator::Load( alpha, beta, beta_squared, step_size, src_meas_data, src_estimate, src_coil_map, src_lambda_map );

	// make sure sizes are compatible
	if( !src_meas_data.Size().Equals( src_estimate.Size() ) )
	{
		GIRLogger::LogError( "TCRIteratorCPU::Load -> src_meas_data is not the same size as src_estimate, aborting!\n" );
		return;
	}

	// load meas_data
	if( meas_data != 0 ) delete[] meas_data;
	meas_data = new float[src_meas_data.NumElements()];
	Order( src_meas_data, meas_data );

	// load estimate
	if( estimate != 0 ) delete[] estimate;
	estimate = new float[src_estimate.NumElements()];
	Order( src_estimate, estimate );

	// load coil_map
	if( coil_map != 0 ) delete[] coil_map;
	coil_map = new float[src_coil_map.NumElements()];
	Order( src_coil_map, coil_map );

	// load lambda_map
	if( lambda_map != 0 ) delete[] lambda_map;
	lambda_map = new float[src_lambda_map.NumElements()];
	Order( src_lambda_map, lambda_map );

	// allocate gradient
	gradient = new float[src_estimate.NumElements()];

	// set max_pixel and pixels_per_thread
	int num_pixels = src_meas_data.NumPixels();
	int pixels_per_thread = (int)ceil( (float)num_pixels / num_threads );

	// initialize thread data
	for( int i = 0; i < num_threads; i++ )
	{
		args[i].thread_idx = i;
		args[i].num_threads = num_threads;
		args[i].pixel_start = i * pixels_per_thread;
		args[i].pixel_length = pixels_per_thread;
		args[i].num_pixels = num_pixels;
		args[i].image_size = src_meas_data.Size().Column * src_meas_data.Size().Line;
		args[i].channel_size = args[i].image_size * temp_dim_size;
		args[i].coil_channel_size = args[i].image_size;
		args[i].slice_size = args[i].channel_size * src_meas_data.Size().Channel;
		args[i].coil_slice_size = args[i].image_size * src_meas_data.Size().Channel;
		args[i].data_size = src_meas_data.Size();
		args[i].temp_dim_size = temp_dim_size;
		args[i].coil_map = coil_map;
		args[i].lambda_map = lambda_map;
		args[i].meas_data = meas_data;
		args[i].estimate = estimate;
		args[i].gradient = gradient;
		args[i].alpha = alpha;
		args[i].beta = beta;
		args[i].beta_squared = beta_squared;
		args[i].step_size = step_size;
	}
}

void TCRIteratorCPU::Unload( MRIData& dest_estimate )
{
	if( estimate == 0 )
		GIRLogger::LogError( "TCRIteratorCPU::Unload -> estimate == 0, unload aborting!\n" );
	else
		Unorder( dest_estimate, estimate );
}

void TCRIteratorCPU::ApplySensitivity()
{
	// create threads
	for( int i = 0; i < num_threads; i++ )
		pthread_create( &pthreads[i], NULL, CPU_ApplySensitivity, (void*)(&args[i]) );
	// join threads
	for( int i = 0; i < num_threads; i++ )
		pthread_join( pthreads[i], NULL );
}

void TCRIteratorCPU::ApplyInvSensitivity() 
{
	// create threads
	for( int i = 0; i < num_threads; i++ )
		pthread_create( &pthreads[i], NULL, CPU_ApplyInvSensitivity, (void*)(&args[i]) );
	// join threads
	for( int i = 0; i < num_threads; i++ )
		pthread_join( pthreads[i], NULL );
}

void TCRIteratorCPU::FFT() 
{
	// create threads
	for( int i = 0; i < num_threads; i++ )
		pthread_create( &pthreads[i], NULL, CPU_FFT, (void*)(&args[i]) );
	// join threads
	for( int i = 0; i < num_threads; i++ )
		pthread_join( pthreads[i], NULL );
}

void TCRIteratorCPU::IFFT() 
{
	// create threads
	for( int i = 0; i < num_threads; i++ )
		pthread_create( &pthreads[i], NULL, CPU_IFFT, (void*)(&args[i]) );
	// join threads
	for( int i = 0; i < num_threads; i++ )
		pthread_join( pthreads[i], NULL );
}

void TCRIteratorCPU::ApplyFidelityDifference()
{
	// create threads
	for( int i = 0; i < num_threads; i++ )
		pthread_create( &pthreads[i], NULL, CPU_ApplyFidelityDifference, (void*)(&args[i]) );
	// join threads
	for( int i = 0; i < num_threads; i++ )
		pthread_join( pthreads[i], NULL );
}

void TCRIteratorCPU::CalcTemporalGradient()
{
	// create threads
	for( int i = 0; i < num_threads; i++ )
		pthread_create( &pthreads[i], NULL, CPU_CalcTemporalGradient, (void*)(&args[i]) );
	// join threads
	for( int i = 0; i < num_threads; i++ )
		pthread_join( pthreads[i], NULL );
}

void TCRIteratorCPU::UpdateEstimate()
{
	// create threads
	for( int i = 0; i < num_threads; i++ )
		pthread_create( &pthreads[i], NULL, CPU_UpdateEstimate, (void*)(&args[i]) );
	// join threads
	for( int i = 0; i < num_threads; i++ )
		pthread_join( pthreads[i], NULL );
}
/*
void Plugin_TCR::GenOrigEstimate() 
{
	// create threads
	for( int i = 0; i < num_threads; i++ )
		pthread_create( &pthreads[i], NULL, Par_GenOrigEstimate, (void*)(&args[i]) );
	// join threads
	for( int i = 0; i < num_threads; i++ )
		pthread_join( pthreads[i], NULL );
}

void* TCRIteratorCPU::Par_GenOrigEstimate( void* args_ptr )
{
	CPUArgs* args = (CPUArgs*) args_ptr;
	int last_pixel = args->pixel_start + args->pixel_length;
	if( last_pixel > args->max_pixel )
		last_pixel = args->max_pixel;

	GIRLogger::LogDebug( "generating original estimate (%d, %d)...\n", args->pixel_start, last_pixel );

	for( int i = args->pixel_start; i < last_pixel; i++ )
	{

		// see if we need to interpolate
		if( fabs( args->k_space[2*i] ) >= 1e-20 || fabs( args->k_space[2*i+1] ) >= 1e-20 )
		{
			args->estimate[2*i] = args->k_space[2*i];
			args->estimate[2*i+1] = args->k_space[2*i+1];
		}
		else
		{
			int image_num = i / args->image_size;
			int phase = image_num % args->phases;
			int phase0 = i - ( phase * args->image_size );

			// check before
			bool found_before = false;
			int before_index = 0;
			for( int j = i-args->image_size; j >= phase0; j -= args->image_size )
			{
				if( fabs( args->k_space[2*j] ) > 1e-20 && fabs( args->k_space[2*j+1] ) > 1e-20 )
				{
					found_before = true;
					before_index = j;
					break;
				}
			}

			// check after
			bool found_after = false;
			int after_index = 0;
			for( int j = i + args->image_size; j < phase0 + (args->image_size*args->phases); j += args->image_size )
			{
				if( fabs( args->k_space[2*j] ) > 1e-20 && fabs( args->k_space[2*j+1] ) > 1e-20 )
				{
					found_after = true;
					after_index = j;
					break;
				}
			}

			// found before and after
			if( found_before && found_after )
			{
				int total_distance = after_index - before_index;
				int before_distance = i - before_index;
				int after_distance = after_index - i;
				args->estimate[2*i] = (float)( args->k_space[2*before_index]*after_distance + args->k_space[2*after_index]*before_distance ) / total_distance;
				args->estimate[2*i+1] = (float)( args->k_space[2*before_index+1]*after_distance + args->k_space[2*after_index+1]*before_distance ) / total_distance;
			}

			// only found before
			else if( found_before )
			{
				args->estimate[2*i] = args->k_space[2*before_index];
				args->estimate[2*i+1] = args->k_space[2*before_index+1];
			}

			// only found after
			else if( found_after )
			{
				args->estimate[2*i] = args->k_space[2*after_index];
				args->estimate[2*i+1] = args->k_space[2*after_index+1];
			}
		}
	}
	GIRLogger::LogDebug( "### estimate done\n" );
}
*/

