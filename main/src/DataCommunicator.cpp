#include "DataCommunicator.h"
#include "MRIData.h"
#include "Serializable.h"
#include "MRIDataComm.h"
#include "GIRConfig.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "GIRLogger.h"

DataCommunicator::DataCommunicator( int new_buffer_size ) :
	buffer_size( new_buffer_size ),
	buffer_occupied_size( 0 ),
	buffer_data_type( SER_EMPTY )
{
	buffer = new char[buffer_size];
}

DataCommunicator::~DataCommunicator()
{
	if( buffer != 0 )
		delete [] buffer;
}

void DataCommunicator::Purge()
{
	buffer_data_type = SER_EMPTY;
	buffer_occupied_size = 0;
}

bool DataCommunicator::SendReconRequest( MRIReconRequest& request )
{
	return SendSerializable( request, SER_RECON_REQUEST );
}

bool DataCommunicator::SendReconAck( MRIReconAck& ack )
{
	return SendSerializable( ack, SER_RECON_ACK );
}

bool DataCommunicator::SendConfig( GIRConfig& config )
{
	return SendSerializable( config, SER_CONFIG );
}

bool DataCommunicator::SendData( MRIData& data )
{
	// send header
	MRIDataHeader header( data.Size(), data.IsComplex() );
	if( !SendDataHeader( header ) )
	{
		GIRLogger::LogError( "DataCommunicator::SendMRIData -> error sending header, aborting SendMRIData!\n" );
		return false;
	}

	// send measurements
	for( int average = 0; average < data.Size().Average; average++ )
	for( int segment = 0; segment < data.Size().Segment; segment++ )
	for( int partition = 0; partition < data.Size().Partition; partition++ )
	for( int repetition = 0; repetition < data.Size().Repetition; repetition++ )
	for( int echo = 0; echo < data.Size().Echo; echo++ )
	for( int slice = 0; slice < data.Size().Slice; slice++ )
	for( int phase = 0; phase < data.Size().Phase; phase++ )
	for( int set = 0; set < data.Size().Set; set++ )
	for( int line = 0; line < data.Size().Line; line++ )
	{
		MRIDimensions index( 0, line, 0, set, phase, slice, echo, repetition, segment, partition, average );
		MRIMeasurement meas( data.Size().Column, data.Size().Channel, index, data.IsComplex() );
		meas.LoadData( data );
		if( !SendMeasurement( meas ) )
		{
			GIRLogger::LogError( "DataCommunicator::SendMRIData -> error sending measurement, aborting SendMRIData!\n" );
			return false;
		}
	}

	// send end signal
	if( !SendEndSignal() )
	{
		GIRLogger::LogError( "DataCommunicator::SendMRIData -> error sending end signal, aborting SendMRIData!\n" );
		return false;
	}

	return true;
}

bool DataCommunicator::SendDataHeader( MRIDataHeader& header )
{
	return SendSerializable( header, SER_DATA_HEADER );
}

bool DataCommunicator::SendMeasurement( MRIMeasurement &meas ) {
	return SendSerializable( meas, SER_MEASUREMENT );
}

bool DataCommunicator::SendEndSignal()
{
	MRIEndSignal end_signal;
	return SendSerializable( end_signal, SER_END_SIGNAL );
}

bool DataCommunicator::ReceiveReconRequest( MRIReconRequest& request )
{
	return ReceiveSerializable( request, SER_RECON_REQUEST );
}

bool DataCommunicator::ReceiveReconAck( MRIReconAck& ack )
{
	return ReceiveSerializable( ack, SER_RECON_ACK );
}

bool DataCommunicator::ReceiveConfig( GIRConfig& config)
{
	return ReceiveSerializable( config, SER_CONFIG );
}

bool DataCommunicator::ReceiveData( MRIData& mri_data )
{

	// receive header
	MRIDataHeader header;
	if( !ReceiveDataHeader( header ) )
	{
		GIRLogger::LogError( "DataCommunicator::ReceiveData -> ReceiveHeader failed! Aborting...\n" );
		return false;
	}
	else
	{
		GIRLogger::LogInfo( "header received...\n" );
		GIRLogger::LogInfo( "\tlines:%d, columns:%d, channels:%d, sets:%d, phases:%d, slices:%d, echos:%d, repetitions:%d, partitions:%d, segments:%d, averages:%d\n", header.Size().Line, header.Size().Column, header.Size().Channel, header.Size().Set, header.Size().Phase, header.Size().Slice, header.Size().Echo, header.Size().Repetition, header.Size().Partition, header.Size().Segment, header.Size().Average );
		GIRLogger::LogInfo( "waiting for measurements...\n" );
	}

	// receive measurements
	std::vector<MRIMeasurement> all_meas;
	MRIMeasurement meas;
	while( ReceiveMeasurement( meas ) )
		all_meas.push_back( meas );

	// find max dimensions
	unsigned int i;
	MRIDimensions max_dims( header.Size() );
	for( i = 0; i < all_meas.size(); i++ )
	{
		for( int j = 0; j < max_dims.GetNumDims(); j++ )
		{
			int max_dim = 1;
			max_dims.GetDim( j, max_dim );
			int meas_dim = 1;
			all_meas[i].index.GetDim( j, meas_dim );
			// add 1 because it is a zero basd index
			meas_dim += 1;
			max_dims.SetDim( j, max( max_dim, meas_dim ) );
		}
	}

	//mri_data = MRIData( header.Size(), header.IsComplex() );
	GIRLogger::LogInfo( "Max Dims: %s\n", max_dims.ToString().c_str() );
	GIRLogger::LogInfo( "Header Dims: %s\n", header.Size().ToString().c_str() );
	mri_data = MRIData( max_dims, header.IsComplex() );

	// fill mri_data
	for( i = 0; i < all_meas.size(); i++ )
	{
		if( !all_meas[i].UnloadData( mri_data ) )
			GIRLogger::LogError( "DataCommunicator::ReceiveData -> meas.UnloadData failed!\n" );
	}

	// make sure there wasn't an error
	if( BufferDataType() == SER_ERROR )
	{
		GIRLogger::LogError( "DataCommunicator::ReceiveData -> buffer_data_type is SER_ERROR, something bad happened! Aborting...\n" );
		return false;
	}
	// make sure the client didn't disconnect
	else if( BufferDataType() == SER_EMPTY )
	{
		GIRLogger::LogError( "DataCommuniator::ReceiveData -> client disconnected prematurely! Aborting...\n" );
		return false;
	}
	// make sure we ended with SER_END_SIGNAL
	else if( BufferDataType() != SER_END_SIGNAL )
	{
		GIRLogger::LogError( "DataCommunicator::ReceiveData -> Something other than a measurement was received before SER_END_SIGNAL! Aborting...\n" );
		return false;
	}
	return true;
}

bool DataCommunicator::ReceiveDataHeader( MRIDataHeader& header )
{
	return ReceiveSerializable( header, SER_DATA_HEADER );
}

bool DataCommunicator::ReceiveMeasurement( MRIMeasurement& meas )
{
	return ReceiveSerializable( meas, SER_MEASUREMENT );
}

bool DataCommunicator::SendBuffer()
{
	// get buffer type
	int ser_type;
	switch( buffer_data_type ) {
		case SER_DATA_HEADER:
			ser_type = SER_DATA_HEADER_FLAG;
			break;
		case SER_MEASUREMENT:
			ser_type = SER_MEASUREMENT_FLAG;
			break;
		case SER_END_SIGNAL:
			ser_type = SER_END_SIGNAL_FLAG;
			break;
		case SER_RECON_REQUEST:
			ser_type = SER_RECON_REQUEST_FLAG;
			break;
		case SER_RECON_ACK:
			ser_type = SER_RECON_ACK_FLAG;
			break;
		case SER_CONFIG:
			ser_type = SER_CONFIG_FLAG;
			break;
		default:
			GIRLogger::LogError( "DataCommunicator::SendBuffer -> unknown serialization type in buffer_data_type, can't send buffer!\n" );
			buffer_data_type = SER_ERROR;
			return false;
			break;
	}

	// send buffer type, size, and data 
	if( SendAll( (char*)&ser_type, sizeof(int) ) == -1 || SendAll( (char*)&buffer_occupied_size, sizeof(int) ) == -1 || SendAll( (char*)buffer, buffer_occupied_size ) == -1 )
	{
		GIRLogger::LogError( "DataCommunicator::SendBuffer -> failed SendAll(), can't send buffer!\n" );
		buffer_data_type = SER_ERROR;
		return false;
	}
	buffer_data_type = SER_EMPTY;
	return true;
}

bool DataCommunicator::ReceiveBuffer()
{
	buffer_data_type = SER_ERROR;

	int type_and_size[2];

	// get data size
	int bytes_received = ReceiveAll( (char*)&type_and_size, sizeof(int) * 2 );
	if( bytes_received == 0  )
	{
		GIRLogger::LogError( "DataCommunicator::ReceiveBuffer -> client disconnected without sending type/size data!\n" );
		return false;
	}
	if( bytes_received == -1  )
	{
		GIRLogger::LogError( "DataCommunicator::ReceiveBuffer -> failed ReceiveAll()!\n" );
		return false;
	}

	// data too big for buffer
	int data_size = type_and_size[1];
	if( data_size > buffer_size )
	{
		GIRLogger::LogError( "DataCommunicator::ReceiveBuffer -> data_size(%d) > buffer_size(%d), aborting!\n", data_size, buffer_size);
		return false;
	}
	// invalid data size
	if( data_size < 1  )
	{
		GIRLogger::LogError( "DataCommunicator::ReceiveBuffer -> bad data size (%d)!\n", data_size );
		return false;
	}
	// get the data 
	int n = ReceiveAll( (char*)buffer, data_size );
	if( n != data_size )
	{
		GIRLogger::LogError( "DataCommunicator::ReceiveBuffer -> bytes received: %d, expected:%d\n", n, data_size );
		return false;
	}

	// set buffer occupied size
	buffer_occupied_size = n;

	// set buffer type
	switch( type_and_size[0] ) {
		case SER_DATA_HEADER_FLAG:
			buffer_data_type = SER_DATA_HEADER;
			break;
		case SER_MEASUREMENT_FLAG:
			buffer_data_type = SER_MEASUREMENT;
			break;
		case SER_END_SIGNAL_FLAG:
			buffer_data_type = SER_END_SIGNAL;
			break;
		case SER_RECON_REQUEST_FLAG:
			buffer_data_type = SER_RECON_REQUEST;
			break;
		case SER_RECON_ACK_FLAG:
			buffer_data_type = SER_RECON_ACK;
			break;
		case SER_CONFIG_FLAG:
			buffer_data_type = SER_CONFIG;
			break;
		default:
			GIRLogger::LogError( "DataCommunicator::ReceiveBuffer -> unrecognized serializable type!\n" );
			buffer_data_type = SER_ERROR;
			break;
	}
	return true;
}

bool DataCommunicator::SendSerializable( MRISerializable& serializable, SerializedDataType data_type )
{
	int ser_size = serializable.Serialize( buffer, buffer_size );
	// make sure serialization succeeded
	if( ser_size == -1 )
	{
		GIRLogger::LogError( "DataCommunicator::SendSerializable -> failed to serialize!\n" );
		buffer_data_type = SER_ERROR;
		return false;
	}
	// send buffer
	buffer_data_type = data_type;
	buffer_occupied_size = ser_size;
	if( !SendBuffer() )
	{
		GIRLogger::LogError( "DataCommunicator::SendSerializable -> error with SendBuffer()!\n" );
		return false;
	}
	return true;
}


bool DataCommunicator::ReceiveSerializable( MRISerializable& serializable, SerializedDataType data_type )
{
	// make sure the buffer is filled
	if( ( buffer_data_type == SER_EMPTY || buffer_data_type == SER_ERROR ) && !ReceiveBuffer() )
	{
		GIRLogger::LogError( "DataCommunicator::ReceiveSerializable -> unable to fill empty/errored buffer, pop failed!\n" );
		return false;
	}
	// make sure the buffer contains correct data
	else if( buffer_data_type != data_type )
	{
		//GIRLogger::LogError( "DataCommunicator::ReceiveSerializable -> buffer does not contain correct data type, pop failed!\n" );
		return false;
	}
	// make sure unserialization is successfull
	else if( serializable.Unserialize( buffer, buffer_occupied_size ) == -1 )
	{
		GIRLogger::LogError( "DataCommunicator::ReceiveSerializable -> failed to unserialize, pop failed!\n" );
		return false;
	}
	// mark the buffer as empty
	else
	{
		buffer_data_type =  SER_EMPTY;
		return true;
	}
}

