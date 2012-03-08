#ifndef TCR_ITERATOR_CUDA_H
#define TCR_ITERATOR_CUDA_H

#include <TCRIterator.h>
#include <MRIData.h>
#include <GIRLogger.h>
#include <KernelCode.h>
#include <vector>
#include <pthread.h>
//#include <vector_types.h>
#include <cuda.h>
#include <cufft.h>

class TCRIteratorCUDA: public TCRIterator
{
	public:
	int cuda_device;

	TCRIteratorCUDA( int new_thread_load, TemporalDimension new_temp_dim );
	~TCRIteratorCUDA();

	virtual void Load( float alpha, float beta, float beta_squared, float step_size, MRIData& src_meas_data, MRIData& src_estimate, MRIData& src_coil_map, MRIData& src_lambda_map );
	virtual void Unload( MRIData& dest_estimate );

	protected:
	virtual void ApplySensitivity();
	virtual void ApplyInvSensitivity();
	virtual void FFT();
	virtual void IFFT();
	virtual void ApplyFidelityDifference();
	virtual void CalcTemporalGradient();
	virtual void UpdateEstimate();

	private:
	// host pointers
	float* h_meas_data;
	float* h_estimate;
	float* h_coil_map;
	float* h_lambda_map;
	// device pointers
	float* d_meas_data;
	float* d_gradient;
	float* d_estimate;
	float* d_coil_map;
	float* d_lambda_map;
	KernelArgs args;

	int thread_load;
	dim3 dim_block;
	dim3 dim_grid;
	cufftHandle plan;
};

#endif
