#ifndef __SERIALIZABLE_H__
#define __SERIALIZABLE_H__

#include "MRIData.h"
#include <string>
#include <map>

class MRISerializable {
	public:
	virtual int Serialize( char* buffer, int buffer_size ) = 0;
	virtual int Unserialize( char* buffer, int buffer_size ) = 0;
	
	virtual ~MRISerializable() {}

	protected:
	bool SerializeInt( int int_value, char*& buffer, int& buffer_size, int& ser_size );
	bool UnserializeInt( int& int_value, char*& buffer, int& buffer_size, int& ser_size );

	bool SerializeString( const std::string& value, char*& buffer, int& buffer_size, int& ser_size );
	bool UnserializeString( std::string& value, char*& buffer, int& buffer_size, int& ser_size );

	bool SerializeMRIDimensions( MRIDimensions& dimensions, char*& buffer, int& buffer_size, int& ser_size  );
	bool UnserializeMRIDimensions( MRIDimensions& dimensions, char*& buffer, int& buffer_size, int& ser_size  );

	private:
};

#endif
