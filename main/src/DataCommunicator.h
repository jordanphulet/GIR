#ifndef __DATA_COMMUNICATOR_H__
#define __DATA_COMMUNICATOR_H__

#include <vector>

class MRIData;
class MRIDataHeader;
class MRIMeasurement;
class MRIReconRequest;
class MRIReconAck;
class MRISerializable;
class GIRConfig;

enum SerializedDataType { SER_EMPTY, SER_DATA_HEADER, SER_MEASUREMENT, SER_END_SIGNAL, SER_RECON_REQUEST, SER_RECON_ACK, SER_CONFIG, SER_ERROR };
const int SER_DATA_HEADER_FLAG = 0;
const int SER_MEASUREMENT_FLAG = 1;
const int SER_END_SIGNAL_FLAG = 2;
const int SER_RECON_REQUEST_FLAG = 3;
const int SER_RECON_ACK_FLAG = 4;
const int SER_CONFIG_FLAG = 5;

class DataCommunicator
{
	public:
	DataCommunicator( int new_buffer_size = 102400 );
	virtual ~DataCommunicator();

	void Purge();

	SerializedDataType BufferDataType() const { return buffer_data_type; }	

	bool SendReconRequest( MRIReconRequest& request );
	bool SendConfig( GIRConfig& config );
	bool SendReconAck( MRIReconAck& ack );
	bool SendData( MRIData& mri_data );
	bool SendDataHeader( MRIDataHeader& header );
	bool SendMeasurement( MRIMeasurement& meas );
	bool SendEndSignal();

	bool ReceiveReconRequest( MRIReconRequest& request );
	bool ReceiveConfig( GIRConfig& config );
	bool ReceiveReconAck( MRIReconAck& ack );
	bool ReceiveData( MRIData& data );
	bool ReceiveDataHeader( MRIDataHeader& header );
	bool ReceiveMeasurement( MRIMeasurement& meas );

	protected:
	virtual int SendAll( char* data, int data_length ) = 0;
	virtual int ReceiveAll( char* data, int data_length ) = 0;

	private:
	char* buffer;
	const int buffer_size;
	int buffer_occupied_size;
	SerializedDataType buffer_data_type;

	bool SendBuffer();
	bool ReceiveBuffer();
	bool SendSerializable( MRISerializable& serializable, SerializedDataType data_type );
	bool ReceiveSerializable( MRISerializable& serializable, SerializedDataType data_type );
};

#endif
