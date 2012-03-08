//-----------------------------------------------------------------------------
//
// Jordan Hulet, University of Utah, 2010
// jordanphulet@gmail.com
//
//-----------------------------------------------------------------------------

#ifndef FilterTool_H
#define FilterTool_H

#include <pthread.h>

class MRIData;

//#ifndef M_PI
//const double M_PI = 3.141592654;
//#endif

class FilterTool {
	public:
		static void Conv2D( MRIData& image, float* kernel, int kernel_rows, int kernel_cols ); 
		static void Conv2D( float* image, float *kernel, float *dest, int image_rows, int image_cols, int kernel_rows, int kernel_cols, bool image_is_complex, bool kernel_is_complex ); 
		//static void FFT1D( float* dest, float* source, int n, bool reverse = false );
		static void FFT1D_COL( MRIData& data_volume, bool reverse = false );
		static void FFT2D( float *dest, float *source, int data_cols, int data_lines, bool reverse = false ); 
		static void FFT2D( MRIData& data_volume, bool reverse = false ); 

		static void FFTShift( MRIData& dest, bool reverse = false );
		static void FFTShift( MRIData& dest, bool shift_lr, bool shift_ud, bool reverse = false );

		static pthread_mutex_t Mutex;
		static void InitMutex();
		static void DestroyMutex();

};

#endif
