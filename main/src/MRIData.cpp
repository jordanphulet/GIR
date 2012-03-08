#include "MRIData.h"
#include <stdio.h>
#include <cstring>
#include <math.h>
#include <sstream>
#include "GIRLogger.h"

MRIDimensions::MRIDimensions():
	Column( 0 ),
	Line( 0 ),
	Channel( 0 ),
	Set( 0 ),
	Phase( 0 ),
	Slice( 0 ),

	Echo( 0 ),
	Repetition( 0 ),
	Segment( 0 ),
	Partition( 0 ),
	Average( 0 )
{}

MRIDimensions::MRIDimensions( int new_column, int new_line, int new_channel, int new_set, int new_phase, int new_slice, int new_echo, int new_repetition, int new_segment, int new_partition, int new_average ):
	Column( new_column ),
	Line( new_line ),
	Channel( new_channel ),
	Set( new_set ),
	Phase( new_phase ),
	Slice( new_slice ),

	Echo( new_echo ),
	Repetition( new_repetition ),
	Segment( new_segment  ),
	Partition( new_partition ),
	Average( new_average )
{}

bool MRIDimensions::IndexInBounds( const MRIDimensions& index ) const
{
	bool index_in_bounds = true;
	if( index.Column >= Column )
	{
		GIRLogger::LogError( "MRIDimensions::IndexInBounds -> column out of bounds!\n" );
		index_in_bounds = false;
	}
	if( index.Line >= Line )
	{
		GIRLogger::LogError( "MRIDimensions::IndexInBounds -> line out of bounds!\n" );
		index_in_bounds = false;
	}
	if( index.Channel >= Channel )
	{
		GIRLogger::LogError( "MRIDimensions::IndexInBounds -> channel out of bounds!\n" );
		index_in_bounds = false;
	}
	if( index.Set >= Set )
	{
		GIRLogger::LogError( "MRIDimensions::IndexInBounds -> set out of bounds (%s)!\n", index.ToString().c_str() );
		index_in_bounds = false;
	}
	if( index.Phase >= Phase )
	{
		GIRLogger::LogError( "MRIDimensions::IndexInBounds -> phase out of bounds!\n" );
		index_in_bounds = false;
	}
	if( index.Slice >= Slice )
	{
		GIRLogger::LogError( "MRIDimensions::IndexInBounds -> slice out of bounds!\n" );
		index_in_bounds = false;
	}
	
	if( index.Echo >= Echo )
	{
		GIRLogger::LogError( "MRIDimensions::IndexInBounds -> Echo out of bounds!\n" );
		index_in_bounds = false;
	}
	if( index.Repetition >= Repetition )
	{
		GIRLogger::LogError( "MRIDimensions::IndexInBounds -> Repetition out of bounds!\n" );
		index_in_bounds = false;
	}
	if( index.Segment >= Segment )
	{
		GIRLogger::LogError( "MRIDimensions::IndexInBounds -> Segment out of bounds!\n" );
		index_in_bounds = false;
	}
	if( index.Partition >= Partition )
	{
		GIRLogger::LogError( "MRIDimensions::IndexInBounds -> Partition out of bounds!\n" );
		index_in_bounds = false;
	}
	if( index.Average >= Average )
	{
		GIRLogger::LogError( "MRIDimensions::IndexInBounds -> Average out of bounds!\n" );
		index_in_bounds = false;
	}

	return index_in_bounds;
}

bool MRIDimensions::Equals( const MRIDimensions& mri_dimensions ) const
{
	bool dimensions_match = true;
	if( mri_dimensions.Column != Column )
	{
		GIRLogger::LogError( "MRIDimensions::Equals -> column mismatch!\n" );
		dimensions_match = false;
	}
	if( mri_dimensions.Line != Line )
	{
		GIRLogger::LogError( "MRIDimensions::Equals -> line mismatch!\n" );
		dimensions_match = false;
	}
	if( mri_dimensions.Channel != Channel )
	{
		GIRLogger::LogError( "MRIDimensions::Equals -> channel mismatch!\n" );
		dimensions_match = false;
	}
	if( mri_dimensions.Set != Set )
	{
		GIRLogger::LogError( "MRIDimensions::Equals -> set mismatch!\n" );
		dimensions_match = false;
	}
	if( mri_dimensions.Phase != Phase )
	{
		GIRLogger::LogError( "MRIDimensions::Equals -> phase mismatch!\n" );
		dimensions_match = false;
	}
	if( mri_dimensions.Slice != Slice )
	{
		GIRLogger::LogError( "MRIDimensions::Equals -> slice mismatch!\n" );
		dimensions_match = false;
	}

	if( mri_dimensions.Echo != Echo )
	{
		GIRLogger::LogError( "MRIDimensions::Equals -> Echo mismatch!\n" );
		dimensions_match = false;
	}
	if( mri_dimensions.Repetition != Repetition )
	{
		GIRLogger::LogError( "MRIDimensions::Equals -> Repetition mismatch!\n" );
		dimensions_match = false;
	}
	if( mri_dimensions.Segment != Segment )
	{
		GIRLogger::LogError( "MRIDimensions::Equals -> Segment mismatch!\n" );
		dimensions_match = false;
	}
	if( mri_dimensions.Partition != Partition )
	{
		GIRLogger::LogError( "MRIDimensions::Equals -> Partition mismatch!\n" );
		dimensions_match = false;
	}
	if( mri_dimensions.Average != Average )
	{
		GIRLogger::LogError( "MRIDimensions::Equals -> Average mismatch!\n" );
		dimensions_match = false;
	}
	return dimensions_match;
}

bool MRIDimensions::GetDim( int dim_index, int& value )
{
	switch( dim_index )
	{
		case 0:
			value = Column;
			break;
		case 1:
			value = Line;
			break;
		case 2:
			value = Channel;
			break;
		case 3:
			value = Set;
			break;
		case 4:
			value = Phase;
			break;
		case 5:
			value = Slice;
			break;
		case 6:
			value = Echo;
			break;
		case 7:
			value = Repetition;
			break;
		case 8:
			value = Segment;
			break;
		case 9:
			value = Partition;
			break;
		case 10:
			value = Average;
			break;
		default:
			GIRLogger::LogError( "MRIDimensions::GetDim -> invalid dim index: %d!\n", dim_index );
			return false;
	}
	return true;
}

bool MRIDimensions::SetDim( int dim_index, int value )
{
	switch( dim_index )
	{
		case 0:
			Column = value;
			break;
		case 1:
			Line = value;
			break;
		case 2:
			Channel = value;
			break;
		case 3:
			Set = value;
			break;
		case 4:
			Phase = value;
			break;
		case 5:
			Slice = value;
			break;
		case 6:
			Echo = value;
			break;
		case 7:
			Repetition = value;
			break;
		case 8:
			Segment = value;
			break;
		case 9:
			Partition = value;
			break;
		case 10:
			Average = value;
			break;
		default:
			GIRLogger::LogError( "MRIDimensions::SetDim -> invalid dim index: %d!\n", dim_index );
			return false;
	}
	return true;
}

std::string MRIDimensions::ToString() const
{
	std::stringstream stream;
	stream << "column: " << Column << ", line: " << Line << ", channel: " << Channel << ", set: " << Set << ", phase: " << Phase << ", slice: " << Slice;
	stream << ", echo: " << Echo << ", repetition: " << Repetition << ", segment: " << Segment<< ", partition: " << Partition << ", average: " << Average;
	return stream.str();
}

MRIData::MRIData():
	data( 0 ),
	size( 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 ),
	is_complex( false ),
	time_data( 0 )
{
	Initialize();
	data = new float[num_elements];
}

MRIData::MRIData( const MRIData& mri_data ):
	data(0),
	size( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ),
	is_complex( false )
{
	Copy( mri_data );
}

MRIData::MRIData( const MRIDimensions& new_size, bool new_is_complex ):
	size( new_size ),
	is_complex( new_is_complex )
{
	Initialize();
	data = new float[num_elements];
}

MRIData::~MRIData()
{
	if( data != 0 )
		delete [] data;
}

MRIData& MRIData::operator = ( const MRIData &mri_data ) {
	Copy( mri_data );
	return *this;
}

void MRIData::Copy( const MRIData& mri_data )
{
	// free old memory if needed
	if( data != 0 )
		delete [] data;

	size = mri_data.Size();
	is_complex = mri_data.IsComplex();

	// copy data
	data = new float[mri_data.NumElements()];
	memcpy( data, mri_data.data, mri_data.NumElements() * sizeof( float ) );

	Initialize();
}

bool MRIData::SetAll( float value )
{
	if( data == 0 )
	{
		GIRLogger::LogError( "MRIData::SetAll-> data NULL!\n" );
		return false;
	}

	if( IsComplex() )
	{
		for( int i = 0; i < NumPixels(); i++ )
		{
			data[2*i] = value;
			data[2*i + 1] = 0;
		}
	}
	else
	{
		for( int i = 0; i < NumPixels(); i++ )
			data[i] = value;
	}
	return true;
}

bool MRIData::Add( float value )
{
	if( data == 0 )
	{
		GIRLogger::LogError( "MRIData::Add-> data NULL!\n" );
		return false;
	}

	if( IsComplex() )
	{
		for( int i = 0; i < NumPixels(); i++ )
		{
			data[2*i] += value;
		}
	}
	else
	{
		for( int i = 0; i < NumPixels(); i++ )
			data[i] += value;
	}
	return true;
}

bool MRIData::Mult( float value )
{
	if( data == 0 )
	{
		GIRLogger::LogError( "MRIData::Mult-> data NULL!\n" );
		return false;
	}

	if( IsComplex() )
	{
		for( int i = 0; i < NumPixels(); i++ )
		{
			data[2*i] *= value;
			data[2*i+1] *= value;
		}
	}
	else
	{
		for( int i = 0; i < NumPixels(); i++ )
			data[i] *= value;
	}
	return true;
}

float MRIData::GetMax()
{
	// find max
	float current_max;
	for( int i = 0; i < NumPixels(); i++ )
	{
		float this_value;
		if( IsComplex() )
		{
			float real = data[2*i];
			float imag = data[2*i + 1];
			this_value = (float)sqrt( real*real + imag*imag );
		}
		else
			this_value = data[i];
		if( i == 0 || this_value > current_max )
			current_max = this_value;
	}
	return current_max;
}

void MRIData::ScaleMax( float new_max )
{
	float current_max = GetMax();
	if( current_max > 0 )
	{
		float scale_factor = new_max / current_max;
		for( int i = 0; i < NumElements(); i++ )
			data[i] *= scale_factor;
	}
}

void MRIData::MirrorColumns() {
	int total_lines = size.Average * size.Partition * size.Segment * size.Repetition * size.Echo * size.Slice * size.Phase * size.Set * size.Channel * size.Line;
	int line_length = ( IsComplex() )? size.Column*2 : size.Column;
	float *line_buffer = new float[line_length];

	for( int line = 0; line < total_lines; line++ ) {
		int offset = line * line_length;
		memcpy( line_buffer, data + offset, line_length * sizeof( float ) );

		for( int column = 0; column < size.Column; column++ ) {
			int from_column = size.Column - column - 1;
			if( IsComplex() ) {
				data[offset+2*column] = line_buffer[2*from_column];
				data[offset+2*column + 1] = line_buffer[2*from_column + 1];
			}
			else
				data[offset+column] = line_buffer[from_column];
		}
	}
	delete [] line_buffer;
}

void MRIData::Initialize()
{
	num_pixels = size.Column * size.Line * size.Channel * size.Set * size.Phase * size.Slice * size.Echo * size.Repetition * size.Segment * size.Partition * size.Average;
	num_elements = ( is_complex )? num_pixels * 2: num_pixels;
	/*
	slice_size = size.Column * size.Line * size.Channel * size.Set * size.Phase;
	phase_size = size.Column * size.Line * size.Channel * size.Set;
	set_size = size.Column * size.Line * size.Channel;
	*/
	channel_size = size.Column * size.Line;
	set_size = channel_size * size.Channel;
	phase_size = set_size * size.Set;
	slice_size = phase_size * size.Phase;

	echo_size = slice_size * size.Slice;
	repetition_size = echo_size * size.Echo;
	partition_size = repetition_size * size.Repetition;
	segment_size = partition_size * size.Partition;
	average_size = segment_size * size.Segment;
}

float* MRIData::GetDataIndex( MRIDimensions& index ) const
{
	return GetDataIndex( index.Column, index.Line, index.Channel, index.Set, index.Phase, index.Slice, index.Echo, index.Repetition, index.Partition, index.Segment, index.Average );
}

float* MRIData::GetDataIndex( int column, int line, int channel, int set, int phase, int slice, int echo, int repetition, int partition, int segment, int average ) const
{
	if( column >= size.Column || line >= size.Line || channel >= size.Channel || set >= size.Set || phase >= size.Phase || slice >= size.Slice || echo >= size.Echo || repetition >= size.Repetition || partition >= size.Partition || segment >= size.Segment || average >= size.Average  )
	{
		GIRLogger::LogError( "MRIData::GetDataIndex-> index out of bounds, index( %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d ), size( %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d )!\n",
			column, line, channel, set, phase, slice, echo, repetition, partition, segment, average,
			size.Column, size.Line, size.Channel, size.Set, size.Phase, size.Slice, size.Echo, size.Repetition, size.Partition, size.Segment, size.Average );
		return 0;
	}
	int offset = average*average_size + segment*segment_size + partition*partition_size + repetition*repetition_size + echo*echo_size + slice*slice_size + phase*phase_size + set*set_size + channel*channel_size + line*size.Column + column;
	float* data_start = ( is_complex )? data + offset*2 : data + offset;
	return data_start;
}

void MRIData::GetMagnitude( MRIData& mag_data )
{
	// if complex_data isn't complex don't do anytying
	if( !IsComplex() )
	{
		mag_data = *this;
		return;
	}

	// initialize mag data
	mag_data = MRIData( Size(), false );

	// iterate through all the pixels
	float* complex_start = GetDataStart();
	float* mag_start = mag_data.GetDataStart();
	for( int i = 0; i < NumPixels(); i++ )
		mag_start[i] = sqrt( complex_start[2*i]*complex_start[2*i] + complex_start[2*i+1]*complex_start[2*i+1] );
}

void MRIData::MakeAbs()
{
	if( IsComplex() )
	{
		for( int i = 0; i < NumPixels(); i++ )
		{
			data[2*i] = sqrt( data[2*i]*data[2*i] + data[2*i+1]*data[2*i+1] );
			data[2*i+1] = 0;
		}
	}
}
