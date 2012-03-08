#ifndef MRI_DATA_COMM
#define MRI_DATA_COMM

#include "Serializable.h"
#include "GIRConfig.h"

class MRIDataHeader: public MRISerializable
{
	public:
	MRIDataHeader() : size( 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ), is_complex( false ) {}
	MRIDataHeader( const MRIDimensions& new_size, bool new_is_complex );

	const MRIDimensions& Size() const { return size; }
	bool IsComplex() { return is_complex; }

	int Serialize( char* buffer, int buffer_size );
	int Unserialize( char* buffer, int buffer_size );

	protected:

	private:
	MRIDimensions size;
	bool is_complex;
};

class MRIMeasurement: public MRISerializable
{
	public:
	int meas_time;
	MRIDimensions index;

	MRIMeasurement();
	MRIMeasurement( int new_columns, int new_channels, const MRIDimensions& new_index, bool is_complex );

	bool IsCompatible( MRIData& other_data ) const;
	bool LoadData( MRIData& other_data );
	bool UnloadData( MRIData& other_data );

	MRIData& GetData() { return mri_data; }
	MRIDimensions& GetIndex() { return index; }

	int Serialize( char* buffer, int buffer_size );
	int Unserialize( char* buffer, int buffer_size );

	void Print();

	protected:

	private:
	MRIData mri_data;

	bool MoveData( MRIData& other_data, bool move_out );
};

class MRIEndSignal: public MRISerializable
{
	public:
	int Serialize( char* buffer, int buffer_size );
	int Unserialize( char* buffer, int buffer_size );
};

class MRIReconRequest: public MRISerializable
{
	public:
	std::string pipeline;
	bool silent;
	GIRConfig config;

	MRIReconRequest(): silent( false ) {}

	int Serialize( char* buffer, int buffer_size );
	int Unserialize( char* buffer, int buffer_size );

	std::string ToString();
};

class MRIReconAck: public MRISerializable
{
	public:
	bool success;
	std::string message;

	MRIReconAck(): success( false ), message( "" ) {}

	int Serialize( char* buffer, int buffer_size );
	int Unserialize( char* buffer, int buffer_size );

	std::string ToString();

	private:
};

#endif
