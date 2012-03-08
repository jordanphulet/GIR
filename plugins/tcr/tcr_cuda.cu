#include <tcr_cuda.h>
#include <GIRLogger.h>
#include <cufft.h>

#define CUDA_THREADS_PER_BLOCK 256

bool CheckCUDAError( char *tag ) {
	cudaError_t error = cudaGetLastError();
	if( error != cudaSuccess) {
		GIRLogger::LogError( "TCR CUDA error, %s: %s!\n", tag, cudaGetErrorString( error ) );
		return false;
	}
	return true;
}

// uses regular dims
__global__ void CUDA_KernelZeroGradient( float* d_gradient ) {
	int i = blockIdx.y*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
	d_gradient[2*i] = 0;
	d_gradient[2*i+1] = 0;
}

// uses regular dims
__global__ void CUDA_KernelAccumulateGradient( float* d_gradient, float* d_gradient_ch, int num_channels ) {
	int i = blockIdx.y*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
	for( int channel = 0; channel < num_channels; channel++ ) {
		int ch_i = (blockIdx.y*num_channels + channel)*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;

		d_gradient[2*i] += d_gradient_ch[2*ch_i] / num_channels;
		d_gradient[2*i+1] += d_gradient_ch[2*ch_i+1] / num_channels;
	}
}

// uses regular dims
__global__ void CUDA_KernelCalcTemporalGradient( float* d_gradient, float* d_estimate, int phase_size, int num_phases, float beta ) {
	//beta = 1;
	int i = blockIdx.y*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;

	int phase = blockIdx.y/phase_size;
	int next_phase = (phase+1) % num_phases;
	next_phase = (next_phase*phase_size)*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
	
	int prev_phase = (phase+num_phases-1) % num_phases;
	prev_phase = (prev_phase*phase_size)*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
	float beta_squared = 0.00001;
	
	float grad1_real = d_estimate[2*i] - d_estimate[2*next_phase];
	float grad1_imag = d_estimate[2*i+1] - d_estimate[2*next_phase+1];
	float grad1_squared = grad1_real*grad1_real + grad1_imag*grad1_imag;
	
	float grad2_real = -d_estimate[2*i] + d_estimate[2*prev_phase];
	float grad2_imag = -d_estimate[2*i+1] + d_estimate[2*prev_phase+1];
	float grad2_squared = grad2_real*grad2_real + grad2_imag*grad2_imag;
	
	grad1_real = grad1_real / sqrt( grad1_squared + beta_squared );
	grad1_imag = grad1_imag / sqrt( grad1_squared + beta_squared );
	
	grad2_real = grad2_real / sqrt( grad2_squared + beta_squared );
	grad2_imag = grad2_imag / sqrt( grad2_squared + beta_squared );
	
	d_gradient[2*i] -= ( -grad1_real + grad2_real ) * beta;
	d_gradient[2*i+1] -= ( -grad1_imag + grad2_imag ) * beta;

	/*
	next_phase = (blockIdx.y + phase_size*(next_phase-phase)) * (gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
	prev_phase = (blockIdx.y + phase_size*(prev_phase-phase)) * (gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
	
	d_gradient[2*i] += (2*d_estimate[2*i] - d_estimate[2*next_phase] - d_estimate[2*prev_phase]) * beta;
	d_gradient[2*i+1] += (2*d_estimate[2*i+1] - d_estimate[2*next_phase+1] - d_estimate[2*prev_phase+1]) * beta;
	*/
}

// uses regular dims
__global__ void CUDA_KernelUpdateEstimate( float* d_gradient, float* d_estimate, float step_size ) {
	int i = blockIdx.y*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
	d_estimate[2*i] -= step_size * d_gradient[2*i];
	d_estimate[2*i+1] -= step_size * d_gradient[2*i+1];
}

// uses channeled dims
__global__ void CUDA_KernelApplySensitivity( float* d_gradient_ch, float* d_estimate, float* d_sensitivity, int num_channels, int max_index ) {
	//todo: load shared memory
	if( blockIdx.x*(blockDim.x) + threadIdx.x <= max_index ) {
		int i = blockIdx.y*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
		int est_i = (blockIdx.y/num_channels)*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
		int sense_i = (blockIdx.y%num_channels)*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
	
		float coil_real = d_sensitivity[2*sense_i];
		float coil_imag = d_sensitivity[2*sense_i + 1];
	
		float source_real = d_estimate[2*est_i];
		float source_imag = d_estimate[2*est_i + 1];
	
		d_gradient_ch[2*i] = source_real*coil_real - source_imag*coil_imag;
		d_gradient_ch[2*i+1] = source_imag*coil_real + source_real*coil_imag;
	}
}

// uses channeled dims
__global__ void CUDA_KernelApplyInvSensitivity( float* d_gradient_ch, float* d_sensitivity, int num_channels, float alpha, int max_index, float scale_factor ) {
	//todo: load shared memory
	if( blockIdx.x*(blockDim.x) + threadIdx.x <= max_index ) {
		int i = blockIdx.y*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
		int sense_i = (blockIdx.y%num_channels)*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
	
		float coil_real = d_sensitivity[2*sense_i];
		float coil_imag = -d_sensitivity[2*sense_i + 1];
	
		float gradient_real = d_gradient_ch[2*i];
		float gradient_imag = d_gradient_ch[2*i+1];
	
		d_gradient_ch[2*i] = alpha * scale_factor * (gradient_real*coil_real - gradient_imag*coil_imag);
		d_gradient_ch[2*i+1] = alpha * scale_factor * (gradient_imag*coil_real + gradient_real*coil_imag);
	}
}

// uses channeled dims
__global__ void CUDA_KernelFidelityDifference( float* d_gradient_ch, float* d_mask, float* d_meas, int max_index ) {
	//todo: load shared memory
	if( blockIdx.x*(blockDim.x) + threadIdx.x <= max_index ) {
		int i = blockIdx.y*(gridDim.x*blockDim.x) + blockIdx.x*(blockDim.x) + threadIdx.x;
		/*
		//int mask_i = blockIdx.x*(blockDim.x) + threadIdx.x;
		d_gradient_ch[2*i] = (d_mask[i] * d_gradient_ch[2*i]) - d_meas[2*i];
		d_gradient_ch[2*i + 1] = (d_mask[i] * d_gradient_ch[2*i + 1]) - d_meas[2*i + 1];
		*/
		//!FIX THIS
		if( fabs(d_meas[2*i]) > 1e-20 || fabs(d_meas[2*i+1]) > 1e-20 ) {
			d_gradient_ch[2*i] = d_gradient_ch[2*i] - d_meas[2*i];
			d_gradient_ch[2*i + 1] = d_gradient_ch[2*i + 1] - d_meas[2*i + 1];
		}
		else {
			d_gradient_ch[2*i] = 0;
			d_gradient_ch[2*i + 1] = 0;
		}
	}
}

//!ONLY HANDLES 1 SLICE!!!
extern "C" bool IterateTCR( float alpha, float beta, float step_size, int iterations, float* estimate, float* gradient_ch, float* gradient, float* sensitivity, float* mask, float* meas, int rows, int cols, int channels, int sets, int phases ) {
	GIRLogger::LogInfo( "GPU : alpha=%f, beta=%f, step_size=%f, iterations=%d\n", alpha, beta, step_size, iterations );

	//cudaSetDevice( 1 );
	//CheckCUDAError( "cudaSetDevice" );

	// Create a 2D FFT plan.
	cufftHandle plan;
	cufftPlan2d(&plan, rows, cols, CUFFT_C2C);
	if( !CheckCUDAError( "cufftPlan2d" ) ) return false;

	float *d_gradient;
	float *d_estimate;
	float *d_gradient_ch;
	float *d_sensitivity;
	float *d_mask;
	float *d_meas;

	int meas_size =  sizeof(float)*cols*rows*channels*sets*phases*2;
	int est_size =   sizeof(float)*cols*rows*sets*phases*2;
	//int mask_size =  sizeof(float)*cols*rows*channels*partitions;
	int mask_size =  sizeof(float)*cols*rows;
	int sense_size = sizeof(float)*cols*rows*channels*2;

	// allocate memory on device
	cudaMalloc( (void**)&d_gradient,    est_size );
	cudaMalloc( (void**)&d_estimate,    est_size );
	cudaMalloc( (void**)&d_gradient_ch, meas_size );
	cudaMalloc( (void**)&d_sensitivity, sense_size );
	cudaMalloc( (void**)&d_mask,        mask_size );
	cudaMalloc( (void**)&d_meas,        meas_size );
	if( !CheckCUDAError( "cudaMallocs" ) ) return false;

	// copy to device
	cudaMemcpy( d_estimate,    estimate,    est_size,   cudaMemcpyHostToDevice ); 
	cudaMemcpy( d_sensitivity, sensitivity, sense_size, cudaMemcpyHostToDevice ); 
	cudaMemcpy( d_mask,        mask,        mask_size,  cudaMemcpyHostToDevice ); 
	cudaMemcpy( d_meas,        meas,        meas_size,  cudaMemcpyHostToDevice ); 
	if( !CheckCUDAError( "cudaMemcpys host->device" ) ) return false;

	// set block/grid dimensions
	int num_blocks = (int)ceil( rows*cols / CUDA_THREADS_PER_BLOCK );
	int max_index = rows*cols - 1;
	dim3 ch_dim_block( CUDA_THREADS_PER_BLOCK );
	dim3 ch_dim_grid( num_blocks, channels*sets*phases);

	dim3 dim_block( CUDA_THREADS_PER_BLOCK );
	dim3 dim_grid( num_blocks, sets*phases);

	int num_items = rows * cols;
	float scale_factor = 1.0 / num_items;

	// iterate
	for( int i = 0; i < iterations; i++ ) {
		GIRLogger::LogInfo( "iteration: %d\n", i+1 );

		// launch kernel to reset gradient to 0
		CUDA_KernelZeroGradient<<< dim_grid, dim_block >>>( d_gradient );
		cudaThreadSynchronize();
		if( !CheckCUDAError( "CUDA_KernelZeroGradient" ) ) return false;

		if( alpha > 1e-20 ) {
			// launch apply sensitivity kernel
			CUDA_KernelApplySensitivity<<< ch_dim_grid, ch_dim_block >>>( d_gradient_ch, d_estimate, d_sensitivity, channels, max_index );
			cudaThreadSynchronize();
			if( !CheckCUDAError( "CUDA_KernelApplySensitivity 1" ) ) return false;

			// launch cufft
			int image;
			for( image = 0; image < channels*sets*phases; image++ ) {
				int offset = image*rows*cols*2;
				cufftExecC2C( plan, (cufftComplex*)(d_gradient_ch+offset), (cufftComplex*)(d_gradient_ch+offset), CUFFT_FORWARD );
			}
			cudaThreadSynchronize();
			if( !CheckCUDAError( "cufftExecC2C 1" ) ) return false;

			// launch difference kernel
			CUDA_KernelFidelityDifference<<< ch_dim_grid, ch_dim_block >>>( d_gradient_ch, d_mask, d_meas, max_index );
			cudaThreadSynchronize();
			if( !CheckCUDAError( "CUDA_KernelFidelityDifference" ) ) return false;

			// launch inverse cufft
			for( image = 0; image < channels*sets*phases; image++ ) {
				int offset = image*rows*cols*2;
				cufftExecC2C( plan, (cufftComplex*)(d_gradient_ch+offset), (cufftComplex*)(d_gradient_ch+offset), CUFFT_INVERSE );
			}
			cudaThreadSynchronize();
			if( !CheckCUDAError( "cufftExecC2C 2" ) ) return false;
	
			// launch inverse apply sensitivity kernel and scale for IFFT
			CUDA_KernelApplyInvSensitivity<<< ch_dim_grid, ch_dim_block >>>( d_gradient_ch, d_sensitivity, channels, alpha, max_index, scale_factor );
			cudaThreadSynchronize();
			if( !CheckCUDAError( "CUDA_KernelApplySensitivity 2" ) ) return false;

			// launch kernel to accumulate gradient
			CUDA_KernelAccumulateGradient<<< dim_grid, dim_block >>>( d_gradient, d_gradient_ch, channels);
			cudaThreadSynchronize();
			if( !CheckCUDAError( "CUDA_KernelAccumulateGradient" ) ) return false;
		}

		if( beta > 1e-20 ) {
			// launch kernel to calculate temporal gradient
			CUDA_KernelCalcTemporalGradient<<< dim_grid, dim_block >>>( d_gradient, d_estimate, sets, phases, beta );
			cudaThreadSynchronize();
			if( !CheckCUDAError( "CUDA_KernelCalcTemporalGradient" ) ) return false;
		}

		// launch kernel to update the estimate
		CUDA_KernelUpdateEstimate<<< dim_grid, dim_block >>>( d_gradient, d_estimate, step_size );
		cudaThreadSynchronize();
		if( !CheckCUDAError( "CUDA_KernelUpdateEstimate" ) ) return false;
	}

	// copy back to host
	cudaMemcpy( estimate, d_estimate, est_size, cudaMemcpyDeviceToHost ); 
	if( !CheckCUDAError( "cudaMemcpy host<-device" ) ) return false;
	cudaMemcpy( gradient, d_gradient, est_size, cudaMemcpyDeviceToHost ); 
	if( !CheckCUDAError( "cudaMemcpy(ch) host<-device" ) ) return false;
	cudaMemcpy( gradient_ch, d_gradient_ch, meas_size, cudaMemcpyDeviceToHost ); 
	if( !CheckCUDAError( "cudaMemcpy(ch) host<-device" ) ) return false;

	// free up device memory
	cudaFree( d_gradient );
	cudaFree( d_estimate );
	cudaFree( d_gradient_ch );
	cudaFree( d_sensitivity );
	cudaFree( d_mask );
	cudaFree( d_meas );
	if( !CheckCUDAError( "cudaFrees" ) ) return false;
	cufftDestroy( plan );
	if( !CheckCUDAError( "cufftDestroy" ) ) return false;

	cudaThreadExit();
	return true;
}
