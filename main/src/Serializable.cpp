#include "Serializable.h"
#include "GIRLogger.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>

bool MRISerializable::SerializeInt( int int_value, char*& buffer, int& buffer_size, int& ser_size )
{
	int int_size = sizeof( int );

	// make sure buffer is big enough
	if( buffer_size < int_size )
	{
		GIRLogger::LogError( "MRISerializeable::SerializeInt -> buffer size too small!\n" );
		return false;
	}
	// serialize
	int* int_buffer = (int*)buffer;
	int_buffer[0] = int_value;

	buffer += int_size;
	ser_size += int_size;
	buffer_size -= int_size;
	return true;
}

bool MRISerializable::UnserializeInt( int& int_value, char*& buffer, int&  buffer_size, int& ser_size )
{
	int int_size = sizeof( int );

	// make sure buffer is big enough
	if( buffer_size < int_size )
	{
		GIRLogger::LogError( "MRISerializeable::UnserializeInt -> buffer size too small!\n" );
		return false;
	}
	// unserialize char_array
	int* int_buffer = (int*)buffer;
	int_value = int_buffer[0];

	buffer += int_size;
	ser_size += int_size;
	buffer_size -= int_size;
	return true;
}

bool MRISerializable::SerializeString( const std::string& value, char*& buffer, int& buffer_size, int& ser_size )
{
	const char* char_array = value.c_str();
	int string_length = (int)strlen( char_array );

	// serialize string size
	if( !SerializeInt( string_length, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRISerializable::SerializeString -> SerializeInt failed, serialize failed!\n" );
		return false;
	}

	// serialize char array
	if( buffer_size < string_length )
	{
		GIRLogger::LogError( "MRISerializable::SerializeString -> buffer too small for char array, serialize failed!\n" );
		return false;
	}
	memcpy( buffer, char_array, string_length );

	buffer += string_length;
	ser_size += string_length;
	buffer_size -= string_length;

	return true;
}

bool MRISerializable::UnserializeString( std::string& value, char*& buffer, int& buffer_size, int& ser_size )
{
	// unserialize string size
	int string_length = 0;
	if( !UnserializeInt( string_length, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "MRISerializable::UnserializeString -> buffer too small for string size int, unserialize failed!\n" );
		return false;
	}

	if( string_length >= 1024 )
	{
		fprintf( stderr, "MRISerializable::UnserializeString -> current string size is limited to 1024 charaters, unserialize failed!\n" );
		return false;
	}

	// unserialize char array
	char char_array[1024];
	char_array[string_length] = 0;
	if( buffer_size < string_length )
	{
		GIRLogger::LogError( "MRISerializable::SerializeString -> buffer too small for char array, serialize failed!\n" );
		return false;
	}
	memcpy( char_array, buffer, string_length );
	value = char_array;

	buffer += string_length;
	ser_size += string_length;
	buffer_size -= string_length;

	return true;
}


bool MRISerializable::SerializeMRIDimensions( MRIDimensions& dimensions, char*& buffer, int& buffer_size, int& ser_size  )
{
	// serialize each dimension
	for( int i = 0; i < dimensions.GetNumDims() ; i++ )
	{
		int dim_size = 0;
		if( !dimensions.GetDim( i, dim_size ) || !SerializeInt( dim_size, buffer, buffer_size, ser_size ) )
		{
			GIRLogger::LogError( "MRISerializable::SerializeMRIDimensions -> SerializeInt failed!\n" );
			return false;
		}
	}
	return true;
}

bool MRISerializable::UnserializeMRIDimensions( MRIDimensions& dimensions, char*& buffer, int& buffer_size, int& ser_size  )
{
	// unserialize each dimension
	for( int i = 0; i < dimensions.GetNumDims(); i++ )
	{
		int dim_size = 0;
		if( !UnserializeInt( dim_size, buffer, buffer_size, ser_size ) || !dimensions.SetDim( i, dim_size ) )
		{
			GIRLogger::LogError( "MRISerializable::UnserializeMRIDimensions -> UnserializeInt failed!\n" );
			return false;
		}
	}
	return true;
}
