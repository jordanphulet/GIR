#include <RadialGridder.h>
#include <MRIData.h>
#include <GIRLogger.h>
#include <math.h>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string.h>
#include <cstdio>

RadialGridder::~RadialGridder()
{
}

bool RadialGridder::Grid( const MRIData& radial_data, MRIData& cart_data, InterpKernelType kernel_type, int kernel_size, bool flatten )
{
	// set up gridding kernel
	Initialize( kernel_type, kernel_size );

	// set up cart_dat
	MRIDimensions cart_size( radial_data.Size().Column, radial_data.Size().Column, radial_data.Size().Channel, radial_data.Size().Set, radial_data.Size().Phase, radial_data.Size().Slice, radial_data.Size().Echo, radial_data.Size().Repetition, radial_data.Size().Partition, radial_data.Size().Segment, radial_data.Size().Average );
	cart_data = MRIData( cart_size, radial_data.IsComplex() );

	// create temporary padded data for gridding
	MRIDimensions padded_size( radial_data.Size().Column+1, radial_data.Size().Column+1, 1, 1, 1, 1, 1, 1, 1, 1, 1 );
	MRIData padded_dest( padded_size, radial_data.IsComplex() );
	// may or may not be used, depending on flatten
	MRIData padded_ones( padded_size, false );

	// calculate some important values
	//double delta_theta = (double)(M_PI / radial_data.Size().Line );
	int center_sample = (int)floor( radial_data.Size().Column/ 2.0f );
	double golden_angle = M_PI * ( ( sqrt( 5 ) - 1 ) / 2 );

	if( repetition_offset != 0 )
		GIRLogger::LogDebug( "### using repetition_offset: %d...\n", repetition_offset );


	// iterate through each view
	for( int average = 0; average < radial_data.Size().Average; average++ )
	for( int segment = 0; segment < radial_data.Size().Segment; segment++ )
	for( int partition = 0; partition < radial_data.Size().Partition; partition++ )
	for( int repetition = 0; repetition < radial_data.Size().Repetition; repetition++ )
	for( int echo = 0; echo < radial_data.Size().Echo; echo++ )
	for( int channel = 0; channel < radial_data.Size().Channel; channel++ )
	for( int set = 0; set < radial_data.Size().Set; set++ )
	for( int phase = 0; phase < radial_data.Size().Phase; phase++ )
	for( int slice = 0; slice < radial_data.Size().Slice; slice++ )
	{
		// clear temporary data
		padded_dest.SetAll( 0 );
		padded_ones.SetAll( 0 );

		// grid each line
		for( int view = 0; view < radial_data.Size().Line; view++ ) 
		{
			float* radial_begin = radial_data.GetDataIndex( 0, view, channel, set, phase, slice, echo, repetition, partition, segment, average ) ;

			// skip empty lines
			float total_signal = 0;
			for( int ro_sample = 0; ro_sample < radial_data.Size().Column; ro_sample++ )
			{
				total_signal += fabs( radial_begin[2*ro_sample] ) + fabs( radial_begin[2*ro_sample+1] );
			}
			if( total_signal < 1e-20 )
				continue;

			double theta_offset = 0;
			double theta = 0;

			// view ordering
			if( view_ordering == VO_JORDAN_JITTER )
			{
				int phases = radial_data.Size().Phase;
				int radial_views = radial_data.Size().Line;

				int center_phase = (int)ceil( phases / 2 ) ;
				int phase_order;
				if( phase % 2 == 0 )
					phase_order = (int)ceil(phase/2);
				else
					phase_order = (int)ceil((phase-1)/2) + center_phase;
	
				double jitter_theta = M_PI / ( radial_views * phases );
				double delta_theta= (double)(M_PI / radial_views );

				theta_offset = jitter_theta * phase_order;
				theta = view * delta_theta + theta_offset;
			}
			else if( view_ordering == VO_GOLDEN_RATIO )
			{
				//int view_index = (repetition+repetition_offset)*radial_data.Size().Line+ view;
				int view_index = (repetition+phase)*radial_data.Size().Line+ view;
				theta = view_index*golden_angle;
				theta = fmod( theta, M_PI );
			}
			else if( view_ordering == VO_GOLDEN_RATIO_NO_PHASE )
			{
				int view_index = view;
				theta = view_index*golden_angle;
				theta = fmod( theta, M_PI );
			}

			double cos_theta = cos( -theta );
			double sin_theta = sin( -theta );

			// splat each sample
			for( int ro_sample = 0; ro_sample < radial_data.Size().Column; ro_sample++ )
			{
				int radius = ro_sample - center_sample;
				double splat_x = radius*cos_theta + center_sample;
				double splat_y = radius*sin_theta + center_sample;
	
				//GIRLogger::LogInfo( "center_sample: %d, splat_x: %f, splat_y: %f\n", center_sample, splat_x, splat_y );
	
				int top = (int)ceil( splat_y );
				int bottom = (int)floor( splat_y );
				int right = (int)ceil( splat_x );
				int left = (int)floor( splat_x );
	
				//GIRLogger::LogInfo( "\ttop: %d, bottom: %d, left: %d, right: %d\n", top, bottom, left, right );
	
				//!this pointer should probably just be incremented to be faster...
				//float* radial = radial_data.GetDataIndex( ro_sample, view, channel, set, phase, slice, echo, repetition, partition, segment, average ) ;
				float* radial = radial_begin + (2*ro_sample);
	
				if( top == bottom && left == right )
				{
					Splat( padded_dest, padded_ones, radial, left, top, splat_x-left, splat_y-top, flatten );
				}
				else if( top == bottom ) 
				{
					Splat( padded_dest, padded_ones, radial, left, top, splat_x-left, splat_y-top, flatten );
					Splat( padded_dest, padded_ones, radial, right, top, splat_x-right, splat_y-top, flatten );
				}
				else if( left == right ) 
				{
					Splat( padded_dest, padded_ones, radial, left, top, splat_x-left, splat_y-top, flatten );
					Splat( padded_dest, padded_ones, radial, left, bottom, splat_x-left, splat_y-bottom, flatten);
				}
				else
				{
					Splat( padded_dest, padded_ones, radial, left, top, splat_x-left, splat_y-top, flatten);
					Splat( padded_dest, padded_ones, radial, right, top, splat_x-right, splat_y-top, flatten);
					Splat( padded_dest, padded_ones, radial, left, bottom, splat_x-left, splat_y-bottom, flatten);
					Splat( padded_dest, padded_ones, radial, right, bottom, splat_x-right, splat_y-bottom, flatten);
				}
			}
		}

		// get rid of padding
		for( int x = 0; x < cart_data.Size().Column; x++ )
		for( int y = 0; y < cart_data.Size().Line; y++ )
		{
			float ones_value = ( flatten )? padded_ones.GetDataIndex( x, y, 0, 0, 0, 0, 0, 0, 0, 0, 0 )[0]: 0;
			// flatten
			if( ones_value > 1e-20 )
			{
				cart_data.GetDataIndex( x, y, channel, set, phase, slice, echo, repetition, partition, segment, average )[0] = padded_dest.GetDataIndex( x, y, 0, 0, 0, 0, 0, 0, 0, 0, 0 )[0] / ones_value;
				if( cart_data.IsComplex() )
					cart_data.GetDataIndex( x, y, channel, set, phase, slice, echo, repetition, partition, segment, average )[1] = padded_dest.GetDataIndex( x, y, 0, 0, 0, 0, 0, 0, 0, 0, 0 )[1] / ones_value;
			}
			// don't flatten
			else
			{
				cart_data.GetDataIndex( x, y, channel, set, phase, slice, echo, repetition, partition, segment, average )[0] = padded_dest.GetDataIndex( x, y, 0, 0, 0, 0, 0, 0, 0, 0, 0 )[0];
				if( cart_data.IsComplex() )
					cart_data.GetDataIndex( x, y, channel, set, phase, slice, echo, repetition, partition, segment, average )[1] = padded_dest.GetDataIndex( x, y, 0, 0, 0, 0, 0, 0, 0, 0, 0 )[1];
			}
		}
	}

	return true;
}

bool RadialGridder::Initialize( InterpKernelType interp_kernel_type, int kernel_size )
{
	// kernel_size must be odd
	if( kernel_size % 2 == 0 )
	{
		GIRLogger::LogError( "RadialGridder::Initialize -> kernel_size must be odd, changed from %d to %d...\n", kernel_size, kernel_size+1 );
		kernel_size++;
	}

	// load kernel
	switch( interp_kernel_type )
	{
		case KERN_TYPE_BILINEAR:
			LoadBilinearKernel( kernel_size );
			break;
		default:
			GIRLogger::LogError( "RadialGridder::Initialize -> attempted to use un-implemented interpolation kernel type!" );
			return false;
	}
	return true;
}

void RadialGridder::LoadBilinearKernel( int kernel_size )
{
	// initialize kernel object
	kernel = MRIData( MRIDimensions( kernel_size, kernel_size, 1, 1, 1, 1, 1, 1, 1, 1, 1 ), false );
	kernel_center = (int)floor( kernel_size / 2 );
	float* kernel_data = kernel.GetDataStart();

	// fill kernel
	for( int x = 0; x < kernel_size; x++ ) 
	{
		float ramp_x  = ( x < kernel_center )? (float)x / kernel_center : 1 - ( (float)(x - kernel_center ) / ( kernel_center ) );
		for( int y = 0; y < kernel_size; y++ ) 
		{
			float ramp_y  = ( y < kernel_center )? (float)y / kernel_center : 1 - ( (float)(y - kernel_center ) / ( kernel_center ) );
			kernel_data[x*kernel_size + y] = ramp_x * ramp_y;
		}
	}
}

void RadialGridder::Splat( MRIData& cartesian, MRIData& weights, float* radial_value, int cart_x, int cart_y, double splat_diff_x, double splat_diff_y, bool flatten )
{
	// find cartesian value
	float* cartesian_value = cartesian.GetDataIndex( cart_x, cart_y, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

	// find kernel value
	int kern_x = (int)round( splat_diff_y * kernel.Size().Column / 2) + kernel_center; 
	int kern_y = (int)round( splat_diff_x * kernel.Size().Column / 2) + kernel_center;
	float* kern_value = kernel.GetDataIndex( kern_x, kern_y, 0, 0, 0, 0, 0, 0, 0, 0, 0 );

	// splat to cartesian 
	cartesian_value[0] += radial_value[0] * kern_value[0];
	if( cartesian.IsComplex() )
		cartesian_value[1] += radial_value[1] * kern_value[0];

	// splat to ones
	if( flatten )
	{
		float* weights_value = weights.GetDataIndex( cart_x, cart_y, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
		weights_value[0] += kern_value[0];
	}
}
