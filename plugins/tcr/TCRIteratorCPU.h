#ifndef TCR_ITERATOR_CPU_H
#define TCR_ITERATOR_CPU_H

#include <TCRIterator.h>
#include <MRIData.h>
#include <GIRLogger.h>
#include <KernelCode.h>
#include <vector>
#include <pthread.h>

class TCRIteratorCPU: public TCRIterator
{
	public:
	TCRIteratorCPU( int new_num_threads, TemporalDimension new_temp_dim ): 
		meas_data( 0 ), 
		gradient( 0 ),
		estimate( 0 ), 
		coil_map( 0 ), 
		lambda_map( 0 ),
		num_threads( new_num_threads ), 
		args( new_num_threads ),
		pthreads( new_num_threads ),
		TCRIterator( new_temp_dim )
	{
		GIRLogger::LogInfo( "TCRIteratorCPU::TCRIteratorCPU -> initializing with %d CPU threads...\n", new_num_threads );
	}

	~TCRIteratorCPU();

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
	float* meas_data;
	float* gradient;
	float* estimate;
	float* coil_map;
	float* lambda_map;
	int num_threads;
	std::vector<KernelArgs> args;
	std::vector<pthread_t> pthreads;
};

#endif
