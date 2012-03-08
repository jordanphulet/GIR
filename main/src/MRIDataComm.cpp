#include "MRIDataComm.h"
#include "GIRLogger.h"
#include <sstream>
#include <cstring>

//ummm..

MRIDataHeader::MRIDataHeader( const MRIDimensions& new_size, bool new_is_complex ):
	size( new_size ),
	is_complex( new_is_complex )
{
}

int MRIDataHeader::Serialize( char* buffer, int buffer_size  )
{
	int ser_size = 0;

	// serialize size
	if( !SerializeMRIDimensions( size, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIDataHeader::Serialize-> couldn't serialize size, serialization failed!" );
		return -1;
	}

	// serialize complexity
	int complexity_int = ( is_complex )? 1: 0;
	if( !SerializeInt( complexity_int, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIDataHeader::Serialize-> couldn't serialize complexity, serialization failed!" );
		return -1;
	}

	return ser_size;
}

int MRIDataHeader::Unserialize( char* buffer, int buffer_size )
{
	int ser_size = 0;

	// unserialize size
	if( !UnserializeMRIDimensions( size, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIDataHeader::Unserialize-> couldn't unserialize size, unserialization failed!" );
		return -1;
	}

	// unserialize complexity
	int complexity_int = 0;
	if( !UnserializeInt( complexity_int, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIDataHeader::Unserialize-> couldn't unserialize complexity, unserialization failed!" );
		return -1;
	}
	is_complex = complexity_int != 0;

	return ser_size;	
}

MRIMeasurement::MRIMeasurement():
	meas_time( -1 ), 
	index( MRIDimensions( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ) ),
	mri_data( MRIDimensions( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ), true )
{
}

MRIMeasurement::MRIMeasurement( int new_columns, int new_channels, const MRIDimensions& new_index, bool is_complex ):
	meas_time( -1 ),
	index( new_index ),
	mri_data( MRIDimensions( new_columns, 1, new_channels, 1, 1, 1, 1, 1, 1, 1, 1 ), is_complex )
{
}

bool MRIMeasurement::IsCompatible( MRIData& other_data ) const
{
	bool compatible = true;
	if( other_data.IsComplex() != mri_data.IsComplex() )
	{
		GIRLogger::LogError( "MRIMeasurement::IsCompatible -> complexity mismatch!\n" );
		compatible = false;
	}
	if( other_data.Size().Column != mri_data.Size().Column )
	{
		GIRLogger::LogError( "MRIMeasurement::IsCompatible -> # samples in other_data doesn't match mri_data!\n" );
		compatible = false;
	}
	if( other_data.Size().Channel != mri_data.Size().Channel )
	{
		GIRLogger::LogError( "MRIMeasurement::IsCompatible -> # channels in other_data doesn't match mri_data!\n" );
		compatible = false;
	}
	if( !other_data.Size().IndexInBounds( index ) )
	{
		GIRLogger::LogError( "MRIMeasurement::IsCompatible -> measurement index out of bounds!\n" );
		compatible = false;
	}
	return compatible;
}

bool MRIMeasurement::MoveData( MRIData& other_data, bool move_out )
{
	//GIRLogger::LogDebug( "###moving data: %s\n", index.ToString().c_str() );
	//GIRLogger::LogDebug( "###other data: %s\n", other_data.Size().ToString().c_str() );
	// make sure other_data is compatible
	if( !IsCompatible( other_data ) )
	{
		GIRLogger::LogError( "MRIMeasurement::MoveData -> mri_data incompatible with other_data!\n" );
		return false;
	}

	int num_elements = ( mri_data.IsComplex() )? 2*mri_data.Size().Column : mri_data.Size().Column;
	for( int i = 0; i < mri_data.Size().Channel; i++ )
	{
		float* mri_data_ind = mri_data.GetDataIndex( 0, 0, i, 0, 0, 0, 0, 0, 0, 0, 0 );
		if( mri_data_ind == 0 )
		{
			GIRLogger::LogError( "MRIMeasurement::MoveData -> invalid index for mri_data!\n" );
			return false;
		}
		float* other_data_ind = other_data.GetDataIndex( 0, index.Line, i, index.Set, index.Phase, index.Slice, index.Echo, index.Repetition, index.Partition, index.Segment, index.Average );
		if( other_data_ind == 0 )
		{
			GIRLogger::LogError( "MRIMeasurement::MoveData -> invalid index for other_data!\n" );
			return false;
		}

		if( move_out )
			memcpy( other_data_ind, mri_data_ind, num_elements*sizeof( float ) );
		else
			memcpy( mri_data_ind, other_data_ind, num_elements*sizeof( float ) );
	}
	return true;
}

bool MRIMeasurement::LoadData( MRIData& other_data )
{
	if( !MoveData( other_data, false ) )
	{
		GIRLogger::LogError( "MRIMeasurement::LoadData -> MRIMeasurement::MoveData failed! Skipping...\n" );
		return false;
	}
	return true;
}

bool MRIMeasurement::UnloadData( MRIData& other_data )
{
	//GIRLogger::LogDebug( "### moving index: %s\n", index.ToString().c_str() );
	//GIRLogger::LogDebug( "### moving size: %s\n", other_data.Size().ToString().c_str() );
	if( !MoveData( other_data, true ) )
	{
		GIRLogger::LogError( "MRIMeasurement::UnloadData -> MRIMeasurement::MoveData failed! Skipping...\n" );
		return false;
	}
	return true;
}

int MRIMeasurement::Serialize( char* buffer, int buffer_size  )
{
	int ser_size = 0;

	// serialize meas_time
	if( !SerializeInt( meas_time, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIMeasurement::Serialize-> couldn't serialize meas_time, serialization failed!" );
		return -1;
	}

	// serialize index
	if( !SerializeMRIDimensions( index, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIMeasurement::Serialize-> couldn't serialize index, serialization failed!" );
		return -1;
	}

	// serialize size
	MRIDimensions size = mri_data.Size();
	if( !SerializeMRIDimensions( size, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIMeasurement::Serialize-> couldn't serialize data size, serialization failed!" );
		return -1;
	}

	// serialize complexity
	int complexity_int = ( mri_data.IsComplex() )? 1: 0;
	if( !SerializeInt( complexity_int, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIMeasurement::Serialize-> couldn't serialize complexity, serialization failed!" );
		return -1;
	}
	
	// serialize data
	int data_size = mri_data.NumElements();
	if( (int)(data_size*sizeof(float)) > (buffer_size-ser_size) )
	{
		GIRLogger::LogError( "MRIMeasurement::Serialize-> buffer too small for data, serialization failed!" );
		return -1;
	}
	memcpy( buffer, mri_data.GetDataStart(), data_size*sizeof( float ) );
	ser_size += data_size*sizeof(float);

	return ser_size;
}

int MRIMeasurement::Unserialize( char* buffer, int buffer_size )
{
	int ser_size = 0;

	// unserialize meas_time
	if( !UnserializeInt( meas_time, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIMeasurement::Unserialize-> couldn't unserialize meas_time, serialization failed!" );
		return -1;
	}

	// unserialize index 
	if( !UnserializeMRIDimensions( index, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIMeasurement::Unserialize-> couldn't unserialize index, unserialization failed!" );
		return -1;
	}

	// unserialize size
	MRIDimensions size;
	if( !UnserializeMRIDimensions( size, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIMeasurement::Unserialize-> couldn't unserialize size, unserialization failed!" );
		return -1;
	}

	// unserialize complexity
	int complexity_int = 0;
	if( !UnserializeInt( complexity_int, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIMeasurement::Unserialize-> couldn't unserialize complexity, unserialization failed!" );
		return -1;
	}
	bool is_complex = complexity_int != 0;

	// create MRIData
	mri_data = MRIData( size, is_complex );

	// unserialize data
	int data_size = mri_data.NumElements();
	if( (int)(data_size*sizeof(float)) > (buffer_size) )
	{
		GIRLogger::LogError( "MRIMeasurement::Unserialize-> buffer too small for data, unserialization failed!" );
		return -1;
	}
	memcpy( mri_data.GetDataStart(), buffer, data_size*sizeof( float ) );
	ser_size += data_size*sizeof(float);

	return ser_size;
}

void MRIMeasurement::Print()
{
	GIRLogger::LogInfo( "line:%d, set:%d, phase:%d, slice:%d, echo:%d, repetition:%d, partition:%d, segment:%d, average:%d\n", index.Line, index.Set, index.Phase, index.Slice, index.Echo, index.Repetition, index.Partition, index.Segment, index.Average );
	// this is not efficient, but works for now
	for( int i = 0; i < mri_data.Size().Channel; i++ )
	{
		float* data = mri_data.GetDataIndex( 0, 0, i, 0, 0, 0, 0, 0, 0, 0, 0 );
		GIRLogger::LogInfo( "\t%d: -> ", i );
		for( int j = 0; j < mri_data.Size().Column; j++ )
		{
			float real = data[2*j];
			float imag = data[2*j+1];
			GIRLogger::LogInfo( "%f+%fi ", real, imag );
		}
		GIRLogger::LogInfo( "\n" );
	}
}

int MRIEndSignal::Serialize( char* buffer, int buffer_size  ) {
	int end_signal_size = 0;
	if( !SerializeInt( 0, buffer, buffer_size, end_signal_size ) )
	{
		GIRLogger::LogError( "MRIEndSignal::Serialize-> SerializeInt failed!\n" );
		return -1;
	}
	return end_signal_size;
}

int MRIEndSignal::Unserialize( char* buffer, int buffer_size )
{
	int end_signal = -1;
	int ser_size = 0;
	if( !UnserializeInt( end_signal, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIEndSignal::Unserialize-> UnserializeInt failed!\n" );
		return -1;
	}
	else if( end_signal != 0 )
	{
		GIRLogger::LogError( "MRIEndSignal::Unserialize-> expected 0 for end signal but got %d, unserialization failed!\n", end_signal );
		return -1;
	}
	return ser_size;
}

int MRIReconRequest::Serialize( char* buffer, int buffer_size  )
{
	int ser_size = 0;

	// serialize pipeline
	if( !SerializeString( pipeline, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIReconRequest::Serialize-> couldn't serialize recon_method, serialization failed!\n" );
		return -1;
	}

	// serialize silent 
	int silent_int = ( silent )? 1: 0;
	if( !SerializeInt( silent_int, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIReconRequest::Serialize-> couldn't serialize silent, serialization failed!\n" );
		return -1;
	}

	// serialize config
	int config_ser_size = 0;
	if( ( config_ser_size = config.Serialize( buffer, buffer_size )  ) < 0 )
	{
		GIRLogger::LogError( "MRIReconRequest::Serialize-> couldn't serialize config, serialization failed!\n" );
		return -1;
	}
	buffer += config_ser_size;
	ser_size += config_ser_size;
	buffer_size -= config_ser_size;

	return ser_size;
}

int MRIReconRequest::Unserialize( char* buffer, int buffer_size  )
{
	int ser_size = 0;

	// unserialize pipeline
	if( !UnserializeString( pipeline, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIReconRequest::Unsrialize-> couldn't unserialize pipeline, unserialization failed!\n" );
		return -1;
	}

	// unserialize silent_int
	int silent_int = 0;
	if( !UnserializeInt( silent_int, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIReconRequest::Unsrialize-> couldn't unserialize silent_int, unserialization failed!\n" );
		return -1;
	}
	silent = silent_int == 1;

	// unserialize config
	int config_ser_size = 0;
	if( ( config_ser_size = config.Unserialize( buffer, buffer_size )  ) < 0 )
	{
		GIRLogger::LogError( "MRIReconRequest::Unserialize-> couldn't unserialize config, serialization failed!\n" );
		return -1;
	}
	buffer += config_ser_size;
	ser_size += config_ser_size;
	buffer_size -= config_ser_size;

	return ser_size;
}

std::string MRIReconRequest::ToString()
{
	std::stringstream stream;
	stream << "---------------MRIReconRequest---------------" << std::endl;
	stream << "pipeline: " << pipeline << std::endl;

	return stream.str();
}

int MRIReconAck::Serialize( char* buffer, int buffer_size  )
{
	int ser_size = 0;
	// serialize success 
	int valid_int = ( success )? 1: 0;
	if( !SerializeInt( valid_int, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIReconAck::Serialize-> couldn't serialize success, serialization failed!\n" );
		return -1;
	}

	// serialize message 
	if( !SerializeString( message, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIReconAck::Serialize-> couldn't serialize message, serialization failed!\n" );
		return -1;
	}

	return ser_size;
}

int MRIReconAck::Unserialize( char* buffer, int buffer_size  )
{
	int ser_size = 0;
	// serialize success 
	int valid_int = 0;
	if( !UnserializeInt( valid_int, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIReconAck::Unserialize-> couldn't unserialize success, serialization failed!\n" );
		return -1;
	}
	success = valid_int == 1;

	// serialize message 
	if( !UnserializeString( message, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRIReconAck::Unserialize-> couldn't unserialize message, serialization failed!\n" );
		return -1;
	}

	return ser_size;
}

std::string MRIReconAck::ToString()
{
	std::stringstream stream;
	stream << "----------------MRIReconAck-----------------" << std::endl;
	stream << "valid request: " << success << std::endl;
	stream << "message:\n\t" << message << std::endl;
	stream << "---------------------------------------------" << std::endl;

	return stream.str();
}
