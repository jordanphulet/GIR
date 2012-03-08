//-----------------------------------------------------------------------------
//
// Jordan Hulet, University of Utah, 2010
// jordanphulet@gmail.com
//
//-----------------------------------------------------------------------------

#include <FilterTool.h>
#include <MRIData.h>
#include <math.h>
#include <fftw3.h>

/*
#ifdef USE_CUDA
	#include "cuda_code.h"
#endif
*/

#include <pthread.h>
#define FFTW_LOCK(a) pthread_mutex_lock( &Mutex ); a; pthread_mutex_unlock( &Mutex );

pthread_mutex_t FilterTool::Mutex;

void FilterTool::Conv2D( MRIData& image_volume, float* kernel, int kernel_rows, int kernel_cols ) {
	int cols = image_volume.Size().Column;
	int lines = image_volume.Size().Line;
	int channels = image_volume.Size().Channel;
	int sets = image_volume.Size().Set;
	int phases = image_volume.Size().Phase;
	int slices = image_volume.Size().Slice;
	int echos = image_volume.Size().Echo;
	int repetitions = image_volume.Size().Repetition;
	int partitions = image_volume.Size().Partition;
	int segments = image_volume.Size().Segment;
	int averages = image_volume.Size().Average;
	
	int im_size = ( image_volume.IsComplex() )? cols*lines*2 : cols*lines;
	//!possible memory leak
	float* im_buffer = new float[im_size];

	for( int channel = 0; channel < channels; channel++ )
	for( int set = 0; set < sets; set++ )
	for( int phase = 0; phase < phases; phase++ )
	for( int slice = 0; slice < slices; slice++ )
	for( int echo = 0; echo < echos; echo++ )
	for( int repetition = 0; repetition < repetitions; repetition++ )
	for( int partition = 0; partition < partitions; partition++ )
	for( int segment = 0; segment < segments; segment++ )
	for( int average = 0; average < averages; average++ )
	{
		float* slice_address = image_volume.GetDataIndex( 0, 0, channel, set, phase, slice, echo, repetition, partition, segment, average );

		// stupidly Conv2D works on data stored in a different -major order than MRIData is stored
		int column, line;
		int volume_index, buffer_index;

		// copy MRIData to buffer
		for( column = 0; column < cols; column++ )
		for( line = 0; line < lines; line++ )
		{
			buffer_index = column*lines + line;
			volume_index = line*cols + column;

			if( image_volume.IsComplex() )
			{
				im_buffer[2*buffer_index] = slice_address[2*volume_index];
				im_buffer[2*buffer_index+1] = slice_address[2*volume_index+1];
			}
			else
				im_buffer[buffer_index] = slice_address[volume_index];
		}

		Conv2D( im_buffer, kernel, im_buffer, lines, cols, kernel_rows, kernel_cols, image_volume.IsComplex(), false );

		// copy buffer to MRIData
		for( column = 0; column < cols; column++ )
		for( line = 0; line < lines; line++ )
		{
			buffer_index = column*lines + line;
			volume_index = line*cols + column;

			if( image_volume.IsComplex() )
			{
				slice_address[2*volume_index] = im_buffer[2*buffer_index];
				slice_address[2*volume_index+1] = im_buffer[2*buffer_index+1];
			}
			else
				slice_address[volume_index] = im_buffer[buffer_index];
		}
	}

	delete [] im_buffer;
}

void FilterTool::Conv2D( float *image, float *kernel, float *dest, int image_rows, int image_cols, int kernel_rows, int kernel_cols, bool image_is_complex, bool kernel_is_complex ) {
	//! convolution with a complex kernel is not yet implemented, it won't work
	if( kernel_is_complex )
		throw "FilterTool::Conv2D -> Convolution with a complex kernel is not yet implemented!";

	int row_pad = (int)ceil( kernel_rows / 2.0 );
	int col_pad = (int)ceil( kernel_cols / 2.0 );
	int padded_rows = (image_rows + 2*row_pad);
	int padded_cols = (image_cols + 2*col_pad);
	int padded_size = padded_cols * padded_rows;

	// create source
	float* source;
	if( image_is_complex )
		source = new float[2*padded_size];
	else
		source = new float[padded_size];

	// copy image to source and put in padding
	int source_col;
	for( source_col = 0; source_col  < padded_cols; source_col++ )
	{
		int image_col = source_col- col_pad;
		for( int source_row = 0; source_row < padded_rows; source_row++ )
		{
			int image_row = source_row - row_pad;

			if( image_row < 0 )
				image_row = 0;
			if( image_row >= image_rows )
				image_row = image_rows - 1;
			if( image_col < 0 )
				image_col = 0;
			if( image_col >= image_cols )
				image_col = image_cols - 1;

			// set value of source
			if( image_is_complex )
			{
				source[2*(source_col*padded_rows + source_row)] = image[2*(image_col*image_rows + image_row)];
				source[2*(source_col*padded_rows + source_row) + 1] = image[2*(image_col*image_rows + image_row) + 1];
			}
			else
				source[source_col*padded_rows + source_row] = image[image_col*image_rows + image_row];
		}
	}

	// perform convolution
	for( source_col = col_pad; source_col  < padded_cols - col_pad; source_col++ )
	{
		int image_col = source_col - col_pad;
		for( int source_row = row_pad; source_row < padded_rows - row_pad; source_row++ )
		{
			int image_row = source_row - row_pad;

			// kernel loop
			float pixel_value = 0;
			float pixel_value_complex = 0;

			for( int kernel_row = 0; kernel_row < kernel_rows; kernel_row++ )
			{
				for( int kernel_col = 0; kernel_col < kernel_cols; kernel_col++ )
				{
					int offset_row = -kernel_row + row_pad - 1;
					int offset_col = -kernel_col + row_pad - 1;

					int offset_source_row = source_row + offset_row;
					int offset_source_col = source_col + offset_col;

					if( image_is_complex ) {
						pixel_value += kernel[kernel_col*kernel_rows + kernel_row] * source[2*(offset_source_col*padded_rows + offset_source_row)];
						pixel_value_complex += kernel[kernel_col*kernel_rows + kernel_row] * source[2*(offset_source_col*padded_rows + offset_source_row) + 1];
					}
					else
						pixel_value += kernel[kernel_col*kernel_rows + kernel_row] * source[offset_source_col*padded_rows + offset_source_row];
				}
			}

			// set value in image
			if( image_is_complex )
			{
				dest[2*(image_col*image_rows + image_row)] = pixel_value;
				dest[2*(image_col*image_rows + image_row) + 1] = pixel_value_complex;
			}
			else
				dest[image_col*image_rows + image_row] = pixel_value;
		}
	}

	delete [] source;
}

void FilterTool::FFT1D_COL( MRIData& data_volume, bool reverse )
{
	// only works on complex data
	if( !data_volume.IsComplex() )
		return;

	int cols = data_volume.Size().Column;
	int lines = data_volume.Size().Line;
	int channels = data_volume.Size().Channel;
	int sets = data_volume.Size().Set;
	int phases = data_volume.Size().Phase;
	int slices = data_volume.Size().Slice;
	int echos = data_volume.Size().Echo;
	int repetitions = data_volume.Size().Repetition;
	int partitions = data_volume.Size().Partition;
	int segments = data_volume.Size().Segment;
	int averages = data_volume.Size().Average;

	double scale_factor = 1.0 / cols;

	// allocate memory for FFT
	fftwf_complex *fft_buffer = (fftwf_complex*) fftwf_malloc( sizeof( fftwf_complex ) * cols );

	// copy data so that the plan can find a algorithm
	float* data = data_volume.GetDataIndex( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
	int i_buffer = 0, i_data = 0;
	while( i_buffer < cols )
	{
		fft_buffer[i_buffer][0] = data[i_data++];
		fft_buffer[i_buffer++][1] = data[i_data++];
	}

	// create plan
	int direction = ( reverse )? FFTW_BACKWARD: FFTW_FORWARD;
	fftwf_plan plan = fftwf_plan_dft_1d( cols, fft_buffer, fft_buffer, direction, FFTW_MEASURE );

	// iterate through all slices
	for( int channel = 0; channel < channels; channel++ )
	for( int set = 0; set < sets; set++ )
	for( int phase = 0; phase < phases; phase++ )
	for( int slice = 0; slice < slices; slice++ )
	for( int echo = 0; echo < echos; echo++ )
	for( int repetition = 0; repetition < repetitions; repetition++ )
	for( int partition = 0; partition < partitions; partition++ )
	for( int segment = 0; segment < segments; segment++ )
	for( int average = 0; average < averages; average++ )
	for( int line = 0; line < lines; line++ )
	{
		// copy data to buffer
		data = data_volume.GetDataIndex( 0, 0, channel, set, phase, slice, echo, repetition, partition, segment, average ) + line*cols*2;
		int i_buffer = 0, i_data = 0;
		while( i_buffer < cols )
		{
			fft_buffer[i_buffer][0] = data[i_data++];
			fft_buffer[i_buffer++][1] = data[i_data++];
		}

		// execute plan
		fftwf_execute( plan );

		// copy data from buffer
		i_buffer = 0, i_data =0;
		while( i_buffer < cols )
		{
			if( reverse )
			{
				data[i_data++] = (float)(fft_buffer[i_buffer][0] * scale_factor);
				data[i_data++] = (float)(fft_buffer[i_buffer++][1] * scale_factor);
			}
			else 
			{
				data[i_data++] = (float)fft_buffer[i_buffer][0];
				data[i_data++] = (float)fft_buffer[i_buffer++][1];
			}
		}
	}

	// free allocated memory
	fftwf_destroy_plan( plan );
	fftwf_free( fft_buffer );
}

void FilterTool::FFT2D( float *dest, float *source, int cols, int lines, bool reverse ) {
	//host_cufft( dest, source, lines, cols, !reverse );
	double scale_factor = 1.0 / ( lines * cols );

	// allocate memory for FFT
	fftwf_complex *fft_buffer;
	FFTW_LOCK( fft_buffer = (fftwf_complex*) fftwf_malloc( sizeof( fftwf_complex ) * cols * lines ); )

	// copy data to buffer
	int i_buffer = 0, i_data = 0;
	while( i_buffer < cols * lines )
	{
		fft_buffer[i_buffer][0] = source[i_data++];
		fft_buffer[i_buffer++][1] = source[i_data++];
	}

	// create and execute plan
	int direction = ( reverse )? FFTW_BACKWARD: FFTW_FORWARD;
	fftwf_plan plan;
	//FFTW_LOCK( plan = fftwf_plan_dft_2d( cols, lines, fft_buffer, fft_buffer, direction, FFTW_ESTIMATE ); )
	FFTW_LOCK( plan = fftwf_plan_dft_2d( lines, cols, fft_buffer, fft_buffer, direction, FFTW_ESTIMATE ); )

	fftwf_execute( plan );

	// copy data from buffer
	i_buffer = 0, i_data =0;
	while( i_buffer < cols * lines )
	{
		if( reverse )
		{
			dest[i_data++] = (float)(fft_buffer[i_buffer][0] * scale_factor);
			dest[i_data++] = (float)(fft_buffer[i_buffer++][1] * scale_factor);
		}
		else 
		{
			dest[i_data++] = (float)fft_buffer[i_buffer][0];
			dest[i_data++] = (float)fft_buffer[i_buffer++][1];
		}
	}

	// free allocated memory
	FFTW_LOCK( fftwf_destroy_plan( plan ); )
	FFTW_LOCK( fftwf_free( fft_buffer ); )
}

void FilterTool::FFT2D( MRIData& data_volume, bool reverse )
{
	// only works on complex data
	if( !data_volume.IsComplex() )
		return;

	int cols = data_volume.Size().Column;
	int lines = data_volume.Size().Line;
	int channels = data_volume.Size().Channel;
	int sets = data_volume.Size().Set;
	int phases = data_volume.Size().Phase;
	int slices = data_volume.Size().Slice;
	int echos = data_volume.Size().Echo;
	int repetitions = data_volume.Size().Repetition;
	int partitions = data_volume.Size().Partition;
	int segments = data_volume.Size().Segment;
	int averages = data_volume.Size().Average;
	double scale_factor = 1.0 / ( lines * cols );

	// allocate memory for FFT
	fftwf_complex *fft_buffer = (fftwf_complex*) fftwf_malloc( sizeof( fftwf_complex ) * cols * lines );

	// copy data so that the plan can find a algorithm
	float *data = data_volume.GetDataIndex( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
	int i_buffer = 0, i_data = 0;
	while( i_buffer < cols * lines )
	{
		fft_buffer[i_buffer][0] = data[i_data++];
		fft_buffer[i_buffer++][1] = data[i_data++];
	}

	// create plan
	int direction = ( reverse )? FFTW_BACKWARD: FFTW_FORWARD;
	//fftwf_plan plan = fftwf_plan_dft_2d( cols, lines, fft_buffer, fft_buffer, direction, FFTW_MEASURE );
	fftwf_plan plan = fftwf_plan_dft_2d( lines, cols, fft_buffer, fft_buffer, direction, FFTW_MEASURE );

	// iterate through all slices
	for( int channel = 0; channel < channels; channel++ )
	for( int set = 0; set < sets; set++ )
	for( int phase = 0; phase < phases; phase++ )
	for( int slice = 0; slice < slices; slice++ )
	for( int echo = 0; echo < echos; echo++ )
	for( int repetition = 0; repetition < repetitions; repetition++ )
	for( int partition = 0; partition < partitions; partition++ )
	for( int segment = 0; segment < segments; segment++ )
	for( int average = 0; average < averages; average++ )
	{
		// copy data to buffer
		data = data_volume.GetDataIndex( 0, 0, channel, set, phase, slice, echo, repetition, partition, segment, average );
		int i_buffer = 0, i_data = 0;
		while( i_buffer < cols * lines )
		{
			fft_buffer[i_buffer][0] = data[i_data++];
			fft_buffer[i_buffer++][1] = data[i_data++];
		}

		// execute plan
		fftwf_execute( plan );

		// copy data from buffer
		i_buffer = 0, i_data =0;
		while( i_buffer < cols * lines )
		{
			if( reverse ) {
				data[i_data++] = (float)(fft_buffer[i_buffer][0] * scale_factor);
				data[i_data++] = (float)(fft_buffer[i_buffer++][1] * scale_factor);
			}
			else {
				data[i_data++] = (float)fft_buffer[i_buffer][0];
				data[i_data++] = (float)fft_buffer[i_buffer++][1];
			}
		}
	}

	// free allocated memory
	fftwf_destroy_plan( plan );
	fftwf_free( fft_buffer );
}

// this could be done more efficiently, especially if you can assume that all
// matricies will have an even number of columns and lines
void FilterTool::FFTShift( MRIData& dest, bool reverse )
{
		FFTShift( dest, true, true, reverse );
}
void FilterTool::FFTShift( MRIData& dest, bool shift_lr, bool shift_ud, bool reverse )
{

	//int center_column = (reverse)? (int)ceil( dest.Size().column / 2 ) : (int)floor( dest.Size().column / 2 );
	//int center_line = (reverse)? (int)ceil( dest.Size().line / 2 ) : (int)floor( dest.Size().line / 2 );

	int center_column = (int)floor( dest.Size().Column / 2 );
	int center_line = (int)floor( dest.Size().Line / 2 );

	if( reverse && dest.Size().Column % 2 == 1 )
		center_column++;
	if( reverse && dest.Size().Line % 2 == 1 )
		center_line++;

	int columns = dest.Size().Column;
	int lines = dest.Size().Line;
	int channels = dest.Size().Channel;
	int sets = dest.Size().Set;
	int phases = dest.Size().Phase;
	int slices = dest.Size().Slice;

	int echos = dest.Size().Echo;
	int repetitions = dest.Size().Repetition;
	int partitions = dest.Size().Partition;
	int segments = dest.Size().Segment;
	int averages = dest.Size().Average;

	// allocate slice buffer
	int buffer_size = ( dest.IsComplex() ) ? lines * columns * 2 : lines * columns;
	float *slice_buffer = new float[buffer_size];

	for( int echo = 0; echo < echos; echo++ )
	for( int repetition = 0; repetition < repetitions; repetition++ )
	for( int partition = 0; partition < partitions; partition++ )
	for( int segment = 0; segment < segments; segment++ )
	for( int average = 0; average < averages; average++ )
	for( int slice = 0; slice < slices; slice++ )
	for( int phase = 0; phase < phases; phase++ )
	for( int set = 0; set < sets; set++ )
	for( int channel = 0; channel < channels; channel++ )
	{
		float * dest_slice = dest.GetDataIndex( 0, 0, channel, set, phase, slice, echo, repetition, partition, segment, average );

		// shift left-right MRIData -> buffer
		int line;
		int column;
		for( line = 0; line < lines; line++ )
		for( column = 0; column < columns; column++ ) {
			int new_column = (shift_lr) ? (column + center_column) % columns : column;
			if( dest.IsComplex() )
			{
				slice_buffer[2*(line*columns + new_column)] = dest_slice[2*(line*columns + column)];
				slice_buffer[2*(line*columns + new_column) + 1] = dest_slice[2*(line*columns + column) + 1];
			}
			else
				slice_buffer[line*columns + new_column] = dest_slice[line*columns + column];
		}

		// shift up-down buffer -> MRIData
		for( line = 0; line < lines; line++ )
		for( column = 0; column < columns; column++ )
		{
			int new_line = (shift_ud)? (line + center_line) % lines : line;
			if( dest.IsComplex() )
			{
				dest_slice[2*(new_line*columns + column)] = slice_buffer[2*(line*columns + column)];
				dest_slice[2*(new_line*columns + column) + 1] = slice_buffer[2*(line*columns + column) + 1];
			}
			else
				dest_slice[new_line*columns + column] = slice_buffer[line*columns + column];
		}
	}

	delete [] slice_buffer;
}

void FilterTool::InitMutex() {
	pthread_mutex_init( &Mutex, NULL );
}

void FilterTool::DestroyMutex() {
	pthread_mutex_destroy( &Mutex );
}
