#include "MRIDataTool.h"
#include "GIRLogger.h"
#include <stdio.h>

bool MRIDataTool::GetCoilSense( const MRIData &estimate, MRIData& coil_sense )
{
	if( !estimate.IsComplex() )
	{
		GIRLogger::LogError( "MRIDataTool::GetCoilSense -> estimate must be complex, aborting!\n" );
		return false;
	}

	MRIDimensions est_size = estimate.Size();
	MRIDimensions sense_size = MRIDimensions( est_size.Column, est_size.Line, est_size.Channel, 1, 1, est_size.Slice, 1, 1, 1, 1, 1 );
	coil_sense = MRIData( sense_size, true );
	coil_sense.SetAll( 0 );

	// sum over phases
	for( int slice = 0; slice < est_size.Slice; slice++ )
	for( int line = 0; line < est_size.Line; line++ )
	for( int column = 0; column < est_size.Column; column++ )
	{
		for( int channel = 0; channel < est_size.Channel; channel++ )
		{
			float* sense_index = coil_sense.GetDataIndex( column, line, channel, 0, 0, slice, 0, 0, 0, 0, 0 );
			for( int phase = 0; phase < est_size.Phase; phase++ )
			{
				float* estimate_index = estimate.GetDataIndex( column, line, channel, 0, phase, slice, 0, 0, 0, 0, 0 );
				sense_index[0] += estimate_index[0];
				sense_index[1] += estimate_index[1];
			}
		}
	}

	// divide out of SSQ
	for( int slice = 0; slice < est_size.Slice; slice++ )
	for( int line = 0; line < est_size.Line; line++ )
	for( int column = 0; column < est_size.Column; column++ )
	{
		// get SSQ
		float ssq = 0;
		for( int channel = 0; channel < est_size.Channel; channel++ )
		{
			float* sense_index = coil_sense.GetDataIndex( column, line, channel, 0, 0, slice, 0, 0, 0, 0, 0 );
			ssq += sense_index[0]*sense_index[0] + sense_index[1]*sense_index[1];
			//ssq += sqrt( sense_index[0]*sense_index[0] + sense_index[1]*sense_index[1] );
		}
		ssq = sqrt( ssq );

		for( int channel = 0; channel < est_size.Channel; channel++ )
		{
			float* sense_index = coil_sense.GetDataIndex( column, line, channel, 0, 0, slice, 0, 0, 0, 0, 0 );
			sense_index[0] /= ssq;
			sense_index[1] /= ssq;
		}
	}

	// convolve map
	//TODO: if we need it...
	float hann_filter[100] =
	{
		0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
		0.00000f, 0.01368f, 0.04833f, 0.08773f, 0.11345f, 0.11345f, 0.08773f, 0.04833f, 0.01368f, 0.00000f,
		0.00000f, 0.04833f, 0.17071f, 0.30988f, 0.40072f, 0.40072f, 0.30988f, 0.17071f, 0.04833f, 0.00000f,
		0.00000f, 0.08773f, 0.30988f, 0.56250f, 0.72738f, 0.72738f, 0.56250f, 0.30988f, 0.08773f, 0.00000f,
		0.00000f, 0.11345f, 0.40072f, 0.72738f, 0.94060f, 0.94060f, 0.72738f, 0.40072f, 0.11345f, 0.00000f,
		0.00000f, 0.11345f, 0.40072f, 0.72738f, 0.94060f, 0.94060f, 0.72738f, 0.40072f, 0.11345f, 0.00000f,
		0.00000f, 0.08773f, 0.30988f, 0.56250f, 0.72738f, 0.72738f, 0.56250f, 0.30988f, 0.08773f, 0.00000f,
		0.00000f, 0.04833f, 0.17071f, 0.30988f, 0.40072f, 0.40072f, 0.30988f, 0.17071f, 0.04833f, 0.00000f,
		0.00000f, 0.01368f, 0.04833f, 0.08773f, 0.11345f, 0.11345f, 0.08773f, 0.04833f, 0.01368f, 0.00000f,
		0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f
	};

	FilterTool::Conv2D( coil_sense, hann_filter, 10, 10 );

	coil_sense.ScaleMax( 1 );


	return true;
}

void MRIDataTool::GetCoilMap( const MRIData &k_space, MRIData& coil_map )
{
	int k_columns = k_space.Size().Column;
	int k_lines = k_space.Size().Line;
	int k_channels = k_space.Size().Channel;
	int k_phases = k_space.Size().Phase;
	int k_slices = k_space.Size().Slice;
	//int k_echos = k_space.Size().Echo;
	int k_repetitions = k_space.Size().Repetition;
	//int k_partitions = k_space.Size().Partition;
	//int k_segments = k_space.Size().Segment;
	//int k_averages = k_space.Size().Average;

	//MRIDimensions coil_dimensions( k_columns, k_lines, k_channels, 1, 1, k_slices, k_echos, k_repetitions, k_partitions, k_segments, k_averages );
	MRIDimensions coil_dimensions( k_columns, k_lines, k_channels, 1, 1, k_slices, 1, 1, 1, 1, 1 );
	coil_map = MRIData( coil_dimensions, true );
	coil_map.SetAll( 0 );

	int norm_factor = k_repetitions * k_phases;

	// average k-space across phases
	//for( int average = 0; average < k_averages; average++ )
	//for( int segment = 0; segment < k_segments; segment++ )
	//for( int partition = 0; partition < k_partitions; partition++ )
	for( int repetition = 0; repetition < k_repetitions; repetition++ )
	//for( int echo = 0; echo < k_echos; echo++ )
	for( int slice = 0; slice < k_slices; slice++ )
	for( int phase = 0; phase < k_phases; phase++ )
	for( int channel = 0; channel < k_channels; channel++ )
	{ 
		float *source_slice = k_space.GetDataIndex( 0, 0, channel, 0, 0, slice, 0, 0, 0, 0, 0 );
		float *dest_slice = coil_map.GetDataIndex( 0, 0, channel, 0, 0, slice, 0, 0, 0, 0, 0 );
		for( int i = 0; i < k_columns*k_lines; i++ )
		{ 
			if( dest_slice[2*i] < 1e-20 && dest_slice[2*i+1] < 1e-20 )
			{
				dest_slice[2*i] = ( source_slice[2*i] / norm_factor );
				dest_slice[2*i+1] = ( source_slice[2*i+1] / norm_factor );
			}
			else
			{
				dest_slice[2*i] += ( source_slice[2*i] / norm_factor );
				dest_slice[2*i+1] +=( source_slice[2*i+1] / norm_factor );
			}
		}
	}

	// generate k_space mask
	int filter_width = 25;
	//!possible memory leak...
	float *k_mask = new float[k_lines*k_columns];
	for( int line = 0; line < k_lines; line++ )
	{
		for( int col = 0; col < k_columns; col++ )
		{
			bool mark_mask = 
				( line+1 < filter_width && col+1 < filter_width ) ||
				( k_lines-line-1 < filter_width && k_columns-col-1 < filter_width ) ||
				( k_lines-line-1 < filter_width && col+1 < filter_width ) ||
				( line+1 < filter_width && k_columns-col-1 < filter_width );
			if( mark_mask )
				k_mask[k_columns*line + col] = 1;
			else
				k_mask[k_columns*line + col] = 0;
		}
	}

	float hann_filter[100] =
	{
		0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f,
		0.00000f, 0.01368f, 0.04833f, 0.08773f, 0.11345f, 0.11345f, 0.08773f, 0.04833f, 0.01368f, 0.00000f,
		0.00000f, 0.04833f, 0.17071f, 0.30988f, 0.40072f, 0.40072f, 0.30988f, 0.17071f, 0.04833f, 0.00000f,
		0.00000f, 0.08773f, 0.30988f, 0.56250f, 0.72738f, 0.72738f, 0.56250f, 0.30988f, 0.08773f, 0.00000f,
		0.00000f, 0.11345f, 0.40072f, 0.72738f, 0.94060f, 0.94060f, 0.72738f, 0.40072f, 0.11345f, 0.00000f,
		0.00000f, 0.11345f, 0.40072f, 0.72738f, 0.94060f, 0.94060f, 0.72738f, 0.40072f, 0.11345f, 0.00000f,
		0.00000f, 0.08773f, 0.30988f, 0.56250f, 0.72738f, 0.72738f, 0.56250f, 0.30988f, 0.08773f, 0.00000f,
		0.00000f, 0.04833f, 0.17071f, 0.30988f, 0.40072f, 0.40072f, 0.30988f, 0.17071f, 0.04833f, 0.00000f,
		0.00000f, 0.01368f, 0.04833f, 0.08773f, 0.11345f, 0.11345f, 0.08773f, 0.04833f, 0.01368f, 0.00000f,
		0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f, 0.00000f
	};

	//!possible memory leak...
	float *smooth_mask = new float[k_lines*k_columns];
	//!note: currently FilterTool::Conv2D shifts the data, so we end up getting a smooth_mask that is off center by 5 pixels...
	FilterTool::Conv2D( k_mask, hann_filter, smooth_mask, k_lines, k_columns, 10, 10, false, false );

	// low pass in k
	//for( int average = 0; average < k_averages; average++ )
	//for( int segment = 0; segment < k_segments; segment++ )
	//for( int partition = 0; partition < k_partitions; partition++ )
	//for( int repetition = 0; repetition < k_repetitions; repetition++ )
	//for( int echo = 0; echo < k_echos; echo++ )
	for( int slice = 0; slice < k_slices; slice++ )
	for( int channel = 0; channel < k_channels; channel++ )
	{ 
		//float *source_slice = smooth_mask;
		float *source_slice = k_mask;
		float *dest_slice = coil_map.GetDataIndex( 0, 0, channel, 0, 0, slice, 0, 0, 0, 0, 0 );
		for( int i = 0; i < k_columns*k_lines; i++ )
		{
			dest_slice[2*i] *= source_slice[i];
			dest_slice[2*i + 1] *= source_slice[i];
		}
	}

	// IFFT so we can create reference image
	FilterTool::FFT2D( coil_map, true );

	// combine coils to create reference image
	//TODO: can we get rid of this MRIData?
	MRIDimensions coil_estimate_dimensions( k_columns, k_lines, k_channels, 1, 1, k_slices, 1, 1, 1, 1, 1 );
	MRIData coil_image_estimate( coil_estimate_dimensions, false );

	//for( int average = 0; average < k_averages; average++ )
	//for( int segment = 0; segment < k_segments; segment++ )
	//for( int partition = 0; partition < k_partitions; partition++ )
	//for( int repetition = 0; repetition < k_repetitions; repetition++ )
	//for( int echo = 0; echo < k_echos; echo++ )
	for( int slice = 0; slice < k_slices; slice++ )
	{
	 	float *dest_slice = coil_image_estimate.GetDataIndex( 0, 0, 0, 0, 0, slice, 0, 0, 0, 0, 0 );
		for( int channel = 0; channel < k_channels; channel++ )
		{ 
			float *source_slice = coil_map.GetDataIndex( 0, 0, channel, 0, 0, slice, 0, 0, 0, 0, 0 );
			for( int i = 0; i < k_columns*k_lines; i++ )
			{
				float real = source_slice[2*i];
				float imag = source_slice[2*i + 1];
				// first channel: initialize
				if( channel == 0 )
					dest_slice[i] = real*real + imag*imag;
				// other channels: add
				else
					dest_slice[i] += real*real + imag*imag;
				// last channel take square-root
				if( channel == k_channels - 1 )
					dest_slice[i] = (float)sqrt( dest_slice[i] );
			}
		}
	}

	// divide out coil estimates
	//for( int average = 0; average < k_averages; average++ )
	//for( int segment = 0; segment < k_segments; segment++ )
	//for( int partition = 0; partition < k_partitions; partition++ )
	//for( int repetition = 0; repetition < k_repetitions; repetition++ )
	//for( int echo = 0; echo < k_echos; echo++ )
	for( int slice = 0; slice < k_slices; slice++ )
	for( int channel = 0; channel < k_channels; channel++ )
	{
		float *ssq_slice = coil_image_estimate.GetDataIndex( 0, 0, 0, 0, 0, slice, 0, 0, 0, 0, 0 );
	 	float *coils_slice = coil_map.GetDataIndex( 0, 0, channel, 0, 0, slice, 0, 0, 0, 0, 0 );

		for( int i = 0; i < k_columns*k_lines; i++ )
		{
			coils_slice[2*i] /= (ssq_slice[i] + 1.0e-20f);
			coils_slice[2*i + 1] /= (ssq_slice[i] + 1.0e-20f);
		}
	}

	delete [] k_mask;
	delete [] smooth_mask;
}

void MRIDataTool::GetSampleMask( const MRIData &k_space, MRIData& sample_mask ) {
	int k_columns = k_space.Size().Column;
	int k_lines = k_space.Size().Line;
	int k_channels = k_space.Size().Channel;
	int k_sets = k_space.Size().Set;
	int k_phases = k_space.Size().Phase;
	int k_slices = k_space.Size().Slice;
	int k_echos = k_space.Size().Echo;
	int k_repetitions = k_space.Size().Repetition;
	int k_partitions = k_space.Size().Partition;
	int k_segments = k_space.Size().Segment;
	int k_averages = k_space.Size().Average;

	MRIDimensions mask_dims( k_columns, k_lines, k_channels, k_sets, k_phases, k_slices, k_echos, k_repetitions, k_partitions, k_segments, k_averages );
	sample_mask = MRIData( mask_dims, false );

	// look for non-empty k-space
	float *k_start = k_space.GetDataStart();
	float *mask_start = sample_mask.GetDataStart();

	for( int i = 0; i < sample_mask.NumPixels(); i++ )
	{
		float real = k_start[2*i];
		float imag = k_start[2*i + 1];
		mask_start[i] = ( fabs(real) > 1e-20 || fabs(imag) > 1e-20 ) ? 1.0f : 0;
	}
}

bool MRIDataTool::GetSSQPhaseContrastEstimate( MRIData &k_space, MRIData& image_estimate ) {
	// get SSQ magnitude image but preserve the phase difference between the 2 sets

	int k_columns = k_space.Size().Column;
	int k_lines = k_space.Size().Line;
	int k_channels = k_space.Size().Channel;
	int k_sets = k_space.Size().Set;
	int k_phases = k_space.Size().Phase;
	int k_slices = k_space.Size().Slice;
	int k_echos = k_space.Size().Echo;
	int k_repetitions = k_space.Size().Repetition;
	int k_partitions = k_space.Size().Partition;
	int k_segments = k_space.Size().Segment;
	int k_averages = k_space.Size().Average;

	if( !k_space.IsComplex() )
	{
		GIRLogger::LogError( "MRIDataTool::GetSSQPhaseContrastEstimate -> k_space isn't complex!\n" );
		return false;
	}

	// only works for PC data with 1 venc...
	// set 1 must be the reference set
	if( k_sets != 2 )
	{
		GIRLogger::LogError( "MRIDataTool::GetSSQPhaseContrastEstimate -> dataset doesn't have dimension sets=2 (ref & venc) as expected for a PC dataset!\n" );
		return false;
	}

	MRIDimensions est_dims( k_columns, k_lines, 1, k_sets, k_phases, k_slices, k_echos, k_repetitions, k_partitions, k_segments, k_averages );
	image_estimate = MRIData( est_dims, true );
	image_estimate.SetAll( 0 );

	// move to image space
	FilterTool::FFT2D( k_space, true );
	
	for( int average = 0; average < k_averages; average++ )
	for( int segment = 0; segment < k_segments; segment++ )
	for( int partition = 0; partition < k_partitions; partition++ )
	for( int repetition = 0; repetition < k_repetitions; repetition++ )
	for( int echo = 0; echo < k_echos; echo++ )
	for( int slice = 0; slice < k_slices; slice++ )
	for( int phase = 0; phase < k_phases; phase++ )
	{

	 	float *dest_set1 = image_estimate.GetDataIndex( 0, 0, 0, 0, phase, slice, echo, repetition, partition, segment, average );
	 	float *dest_set2 = image_estimate.GetDataIndex( 0, 0, 0, 1, phase, slice, echo, repetition, partition, segment, average );

		for( int i = 0; i < k_columns * k_lines; i++ )
		{
			float value_mag_set1 = 0;
			float value_mag_set2 = 0;
			float value_pc = 0;

			for( int channel = 0; channel < k_channels; channel++ )
			{

	 			float *source_set1 = k_space.GetDataIndex( 0, 0, channel, 0, phase, slice, echo, repetition, partition, segment, average );
	 			float *source_set2 = k_space.GetDataIndex( 0, 0, channel, 1, phase, slice, echo, repetition, partition, segment, average );

				float real1 = source_set1[2*i];
				float imag1 = source_set1[2*i + 1];

				float real2 = source_set2[2*i];
				float imag2 = source_set2[2*i + 1];

				float mag1 = real1*real1 + imag1*imag1;
				float mag2 = real2*real2 + imag2*imag2;

				value_mag_set1 += mag1;
				value_mag_set2 += mag2;

				float phase_difference_real = (real1*real2 + imag1*imag2) / mag2; 
				float phase_difference_imag = (imag1*real2 - real1*imag2) / mag2; 
				
				if( phase_difference_real != 0 )
					value_pc += atan2( phase_difference_imag, phase_difference_real ) * mag1;
				else
					value_pc += 3.14159 / 2 * mag1;
			}

			if( value_mag_set1 != 0 )
				value_pc /= value_mag_set1;
			else
				value_pc = 0;

			value_mag_set1 = sqrt( value_mag_set1 );
			value_mag_set2 = sqrt( value_mag_set2 );

			// reference set has 0 phase
			dest_set1[2*i] = value_mag_set1;
			dest_set1[2*i + 1] = 0;

			// compute phase for venc
			float venc_real = value_mag_set2 * cos( -value_pc );
			float venc_imag = value_mag_set2 * sin( -value_pc );

			dest_set2[2*i] = venc_real;
			dest_set2[2*i + 1] = venc_imag;
		}
	}

	// move back to k-space
	FilterTool::FFT2D( k_space, false );

	return true;
}

void MRIDataTool::GetSSQImage( const MRIData &k_space, MRIData& image_estimate )
{
	int k_columns = k_space.Size().Column;
	int k_lines = k_space.Size().Line;
	int k_channels = k_space.Size().Channel;
	int k_sets = k_space.Size().Set;
	int k_phases = k_space.Size().Phase;
	int k_slices = k_space.Size().Slice;
	int k_echos = k_space.Size().Echo;
	int k_repetitions = k_space.Size().Repetition;
	int k_partitions = k_space.Size().Partition;
	int k_segments = k_space.Size().Segment;
	int k_averages = k_space.Size().Average;

	MRIDimensions est_dims( k_columns, k_lines, 1, k_sets, k_phases, k_slices, k_echos, k_repetitions, k_partitions, k_segments, k_averages );
	image_estimate = MRIData( est_dims, true );
	//!possible memory leak...
	float *data_buffer = new float[k_columns*k_lines*2];

	// combine coils
	for( int average = 0; average < k_averages; average++ )
	for( int segment = 0; segment < k_segments; segment++ )
	for( int partition = 0; partition < k_partitions; partition++ )
	for( int repetition = 0; repetition < k_repetitions; repetition++ )
	for( int echo = 0; echo < k_echos; echo++ )
	for( int slice = 0; slice < k_slices; slice++ )
	for( int phase = 0; phase < k_phases; phase++ )
	for( int set = 0; set < k_sets; set++ )
	{
	 	float *dest_slice = image_estimate.GetDataIndex( 0, 0, 0, set, phase, slice, echo, repetition, partition, segment, average );
		for( int channel = 0; channel < k_channels; channel++ )
		{ 

			float *source_slice = k_space.GetDataIndex( 0, 0, channel, set, phase, slice, echo, repetition, partition, segment, average );
			FilterTool::FFT2D( data_buffer, source_slice, k_columns, k_lines, true );

			for( int i = 0; i < k_columns*k_lines; i++ )
			{
				float real = data_buffer[2*i];
				float imag = data_buffer[2*i + 1];

				// first channel: initialize
				if( channel == 0 )
				{
					dest_slice[2*i] = real*real + imag*imag;
					dest_slice[2*i + 1] = 0;
				}
				// other channels: add
				else
					dest_slice[2*i] += real*real + imag*imag;
				// last channel take square-root
				if( channel == k_channels - 1 )
					dest_slice[2*i] = (float)sqrt( dest_slice[2*i] );
			}
		}
	}

	delete [] data_buffer;
}

bool MRIDataTool::TemporallyInterpolateKSpace( const MRIData &k_space, MRIData& interp_k ) {
	if( !k_space.IsComplex() )
	{
		GIRLogger::LogError( "MRIDataTool::TemporallyInterpolateKSpace -> k_space was expected to be complex and it isn't!\n" );
		return false;
	}

	// clone k_space
	interp_k = MRIData( k_space );

	int k_columns = k_space.Size().Column;
	int k_lines = k_space.Size().Line;
	int k_channels = k_space.Size().Channel;
	int k_sets = k_space.Size().Set;
	int k_phases = k_space.Size().Phase;
	int k_slices = k_space.Size().Slice;
	int k_echos = k_space.Size().Echo;
	int k_repetitions = k_space.Size().Repetition;
	int k_partitions = k_space.Size().Partition;
	int k_segments = k_space.Size().Segment;
	int k_averages = k_space.Size().Average;

	GIRLogger::LogDebug( "### phases: %d\n", k_phases );

	int phase_size = k_columns * k_lines * k_channels * k_sets * 2;

	// loop through everything but phase
	for( int average = 0; average < k_averages; average++ )
	for( int segment = 0; segment < k_segments; segment++ )
	for( int partition = 0; partition < k_partitions; partition++ )
	for( int repetition = 0; repetition < k_repetitions; repetition++ )
	for( int echo = 0; echo < k_echos; echo++ )
	for( int slice = 0; slice < k_slices; slice++ )
	for( int channel = 0; channel < k_channels; channel++ )
	for( int set = 0; set < k_sets; set++ )
	for( int i = 0; i < k_columns*k_lines; i++ )
	{
		float* origin_pixel = interp_k.GetDataIndex( 0, 0, channel, set, 0, slice, echo, repetition, partition, segment, average ) + 2*i;
		int last_filled_phase = -1;

		for( int phase = 0; phase < k_phases; phase++ )
		{
			float* this_pixel = origin_pixel + phase_size*phase;
			float* last_filled_pixel = origin_pixel + phase_size*last_filled_phase;
	
			// filled phase
			if( fabs( this_pixel[0] ) > 1e-20 || fabs( this_pixel[1] ) > 1e-20 )
			{
				// no previously filled phase
				if( last_filled_phase == -1 )
				{
					for( int j = last_filled_phase+1; j < phase; j++ )
					{
						float* scan_pixel = origin_pixel + phase_size*j;
						scan_pixel[0] = this_pixel[0];
						scan_pixel[1] = this_pixel[1];
					}
				}
				// previously filled phase
				else
				{
					int phase_distance = phase - last_filled_phase;
					// real
					float real_phase_difference = this_pixel[0] - last_filled_pixel[0];
					float real_step = real_phase_difference / (float)phase_distance;
					// imaginary
					float imag_phase_difference = this_pixel[1] - last_filled_pixel[1];
					float imag_step = imag_phase_difference / (float)phase_distance;
					// linear interpolation
					int offset = 1;
					for( int j = last_filled_phase+1; j < phase; j++ )
					{
						float* scan_pixel = origin_pixel + phase_size*j;
						scan_pixel[0] = last_filled_pixel[0] + offset*real_step; 
						scan_pixel[1] = last_filled_pixel[1] + offset*imag_step; 
						offset++;
					}
				}
	
				last_filled_phase = phase;
			}
		}

		// finish filling if last_filled_phase wasn't the last phase
		if( last_filled_phase != k_phases-1 && last_filled_phase != -1 )
		{
			float* last_filled_pixel = origin_pixel + phase_size*last_filled_phase;
			for( int j = last_filled_phase+1; j < k_phases; j++ ) {
				float* scan_pixel = origin_pixel + phase_size*j;
				scan_pixel[0] = last_filled_pixel[0];
				scan_pixel[1] = last_filled_pixel[1];
			}
		}
	}
	return true;
}

void MRIDataTool::GetPhaseContrastLambdaMap( const MRIData &image_estimate, MRIData& lambda_map )
{
	int est_columns = image_estimate.Size().Column;
	int est_lines = image_estimate.Size().Line;
	int est_channels = image_estimate.Size().Channel;
	int est_sets = image_estimate.Size().Set;
	int est_phases = image_estimate.Size().Phase;

	// only works for PC data with 1 venc...
	// set 1 must be the reference set
	if( est_sets != 2 )
		throw "MRIDataTool::GetSSQPhaseContrastEstimate -> dataset doesn't have dimension sets=2 (ref & venc) as expected for a PC dataset!";

	// must be complex
	if( !image_estimate.IsComplex() )
		throw "MRIDataTool::GetSSQPhaseContrastEstimate -> ssq_est isn't complex!";

	// single image only for now...
	MRIDimensions lambda_dims( est_columns, est_lines, 1, 1, 1, 1, 1, 1, 1, 1, 1 );
	lambda_map = MRIData( lambda_dims, false );
	lambda_map.SetAll( 0 );

	int channel_size = est_columns * est_lines * 2;
	int set_size = channel_size * est_channels;
	int phase_size = set_size * 2;

	for( int i = 0; i < est_columns*est_lines; i++ )
	{
		float *origin_pixel1 = image_estimate.GetDataStart() + 2*i;
		float *origin_pixel2 = origin_pixel1 + set_size;
		float *origin_lambda = lambda_map.GetDataStart() + i;

		float mag_diff = 0;
		for( int phase = 0; phase < est_phases; phase++ )
		{
			float lambda = 0;
			for( int channel = 0; channel < est_channels; channel++ )
			{
				int offset = phase_size*phase + channel*channel_size;
				float* this_pixel1 = origin_pixel1 + offset;
				float* this_pixel2 = origin_pixel2 + offset;

				float real_diff = this_pixel1[0] - this_pixel2[0];
				float imag_diff = this_pixel1[1] - this_pixel2[1];

				//mag_diff += sqrt( real_diff*real_diff + imag_diff*imag_diff );
				lambda += real_diff*real_diff + imag_diff*imag_diff;
			}
			mag_diff += sqrt( lambda );
			//mag_diff = lambda;
		}
		*origin_lambda = mag_diff;
	}

	float kernel[25] = 
	{
		0.0069444, 0.0208333, 0.0277778, 0.0208333, 0.0069444,
		0.0208333, 0.0625000, 0.0833333, 0.0625000, 0.0208333,
		0.0277778, 0.0833333, 0.1111111, 0.0833333, 0.0277778,
		0.0208333, 0.0625000, 0.0833333, 0.0625000, 0.0208333,
		0.0069444, 0.0208333, 0.0277778, 0.0208333, 0.0069444
	}; 

	FilterTool::Conv2D( lambda_map, kernel, 5, 5 );
	lambda_map.Add( -lambda_map.GetMax() );
	lambda_map.Mult( -1 );
	lambda_map.ScaleMax( 1 );
}
