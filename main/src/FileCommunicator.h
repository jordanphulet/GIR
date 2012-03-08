#ifndef __FILE_DATA_COMMUNICATOR_H__
#define __FILE_DATA_COMMUNICATOR_H__

#include "DataCommunicator.h"
#include <string>

class MRIData;

class FileCommunicator : public DataCommunicator
{
	public:
	FileCommunicator( int new_buffer_size = 102400 );
	~FileCommunicator();

	bool OpenOutput( const char* output_path );
	bool OpenInput( const char* input_path );

	bool OpenOutputFD( int output_fd );
	bool OpenInputFD( int input_fd );

	protected:
	virtual int SendAll( char* data, int data_length );
	virtual int ReceiveAll( char* data, int data_length );

	void CloseOutput();
	void CloseInput();

	FILE* output;
	FILE* input;
};

#endif
