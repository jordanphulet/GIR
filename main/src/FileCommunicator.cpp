#include "FileCommunicator.h"
#include "GIRLogger.h"
#include <cstdio>
#include <stdio.h>

FileCommunicator::FileCommunicator( int new_buffer_size ) :
	DataCommunicator( new_buffer_size ),
	output( 0 ),
	input( 0 )
{}

FileCommunicator::~FileCommunicator()
{
	CloseOutput();
	CloseInput();
}

bool FileCommunicator::OpenOutput( const char* output_path )
{
	CloseOutput();
	output = fopen( output_path, "w" );
	return output != 0;
}

bool FileCommunicator::OpenInput( const char* input_path )
{
	CloseInput();
	input = fopen( input_path, "r" );
	return input != 0;
}

bool FileCommunicator::OpenOutputFD( int output_fd )
{
	CloseOutput();
	output = fdopen( output_fd, "w" );
	if( output == 0 )
	{
		GIRLogger::LogError( "FileCommunicator::OpenOutputFD -> fdopen failed!\n" );
		return false;
	}
	return true;
}

bool FileCommunicator::OpenInputFD( int input_fd )
{
	CloseInput();
	input = fdopen( input_fd, "r" );
	if( input == 0 )
	{
		GIRLogger::LogError( "FileCommunicator::OpenInputFD -> fdopen failed!\n" );
		return false;
	}
	return true;
}

int FileCommunicator::SendAll( char* data, int data_length )
{
	// make sure output is open
	if( output == 0 )
	{
		GIRLogger::LogError( "FileCommunicator::SendAll -> output was never opened!\n" );
		return false;
	}

	// write the data
	if( (int)fwrite( data, 1, data_length, output ) != data_length )
	{
		GIRLogger::LogError( "FileCommunicator::SendAll -> fwrite failed!\n" );
		return false;
	}

	// flush
	fflush( output );

	return data_length;
}

int FileCommunicator::ReceiveAll( char* buffer, int data_length )
{
	// make sure input is open
	if( input == 0 )
	{
		GIRLogger::LogError( "FileCommunicator::ReceiveAll -> input was never opened!\n" );
		return false;
	}

	// read the data
	if( (int)fread( buffer, 1, data_length, input ) != data_length )
	{
		GIRLogger::LogError( "FileCommunicator::ReceiveAll -> fread failed!\n" );
		return false;
	}

	return data_length;
}

void FileCommunicator::CloseOutput()
{
	if( output != 0 )
	{
		fclose( output );
		output = 0;
	}
}

void FileCommunicator::CloseInput()
{
	if( input != 0 )
	{
		fclose( input );
		input = 0;
	}
}
