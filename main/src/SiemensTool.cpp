#include <SiemensTool.h>
#include <MRIData.h>
//#include <Serializable.h>
#include <MRIDataComm.h>
#include <GIRLogger.h>
#include <GIRUtils.h>
#include <vector>
#include <fstream>
#include <stdint.h>
#include <math.h>

bool SiemensTool::LoadDatFile( std::string dat_file_path, MRIDataHeader& header, std::vector<MRIMeasurement>& meas_vector )
{
	// make sure the file is readable
	if( !GIRUtils::FileIsReadable( dat_file_path.c_str() ) )
	{
		GIRLogger::LogError( "SiemensTool::LoadDatFile -> dat file: %s isn't readable!\n", dat_file_path.c_str() );
		return false;
	}

	// open the file
	std::fstream file_stream;
	file_stream.open( dat_file_path.c_str(), std::fstream::in | std::fstream::binary );
	if( !file_stream.is_open() )
	{
		GIRLogger::LogError( "SiemensTool::LoadDatFile -> unable to upen file: %s!\n", dat_file_path.c_str() );
		return false;
	}
	file_stream.seekg( 0, std::ios::beg );

	// go to start of measurement data
	uint32_t data_start;
	file_stream.read( (char*)&data_start, sizeof( uint32_t ) );
	if( file_stream.fail() )
	{
		GIRLogger::LogError( "SiemensTool::LoadDatFile -> unable read measurement data offset!\n", dat_file_path.c_str() );
		return false;
	}
	file_stream.seekg( data_start, std::ios::beg );

	// read each measurement
	MRIDimensions data_dims( 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1 );
	while( ReadMeas( file_stream, data_dims, meas_vector ) ) {}

	file_stream.close();

	//GIRLogger::LogDebug( "### read %d measurements\n", meas_vector.size() );
	//GIRLogger::LogDebug( "### size: %s\n", data_dims.ToString().c_str() );

	// create header
	header = MRIDataHeader( data_dims, true );

	return true;
}

bool SiemensTool::ReadMeas( std::fstream& file_stream, MRIDimensions& data_dims, std::vector<MRIMeasurement>& meas_vector )
{
	// get header for channel 0
	MeasHeader meas_header;
	if( !GetMeasHeader( file_stream, meas_header, 0 ) )
		return false;

	UpdateDims( data_dims, meas_header );

	// make sure num samples are consistent, last line will be too short...
	if( data_dims.Column == 0 )
		data_dims.Column = meas_header.samples;
	else if( data_dims.Column != meas_header.samples )
	{
		//GIRLogger::LogWarning( "SiemensTool::ReadMeas -> meas_header.samples does not match previous definition of data_dims.Column, it looks like we are done...\n" );
		return false;
	}

	// make sure num channels are consistent
	if( data_dims.Channel == 0 )
		data_dims.Channel = meas_header.channels;
	else if( data_dims.Channel != meas_header.channels )
	{
		GIRLogger::LogError( "SiemensTool::ReadMeas -> meas_header.samples was %d, but expected %d!\n", meas_header.channels, data_dims.Channel );
		return false;
	}

	// create the measurement
	//! segment not implemented
	MRIDimensions index( 0, meas_header.line, 0, meas_header.set, meas_header.phase, meas_header.slice, meas_header.multi_echo, meas_header.repeat, 0, meas_header.partition, meas_header.average );

	//GIRLogger::LogDebug( "### sending %s\n", index.ToString().c_str() );

	MRIMeasurement meas( meas_header.samples, meas_header.channels, index, true );
	meas.meas_time = meas_header.time;

	// copy channel 0 data
	if( !CopyData( meas, file_stream, meas_header.samples, 0 ) )
		return false;

	// load the rest of the channels
	for( int channel = 1; channel < meas_header.channels; channel++ )
	{
		MeasHeader this_header;
		// get header
		if( !GetMeasHeader( file_stream, this_header, channel ) )
			return false;

		// make sure it is the right size
		if(
			this_header.samples != meas_header.samples ||
			this_header.line != this_header.line ||
			this_header.set != this_header.set ||
			this_header.phase != this_header.phase ||
			this_header.slice != this_header.slice ||

			this_header.multi_echo != this_header.multi_echo ||
			this_header.repeat != this_header.repeat ||
			this_header.partition != this_header.partition ||
			//this_header.segment != this_header.segment ||
			this_header.average != this_header.average
		)
		{
			GIRLogger::LogError( "SiemensTool::ReadMeas -> measurement header for channel %d doesn't match header for channel 0!\n", channel );
			return false;
		}

		// copy channel data
		if( !CopyData( meas, file_stream, meas_header.samples, channel ) )
			return false;
	}

	meas_vector.push_back( meas );

	return !file_stream.eof() && !file_stream.fail();
}

bool SiemensTool::GetMeasHeader( std::fstream& file_stream, MeasHeader& meas_header, int expected_channel )
{
	// read meas header
	file_stream.read( (char*)&meas_header, sizeof( meas_header ) );
	if( file_stream.fail() )
	{
		GIRLogger::LogError( "SiemensTool::GetMeasHeader -> failed to read measurement header!\n" );
		return false;
	}

	// make sure it is the expected channel
	if( meas_header.channel != expected_channel )
	{
		GIRLogger::LogError( "SiemensTool::GetMeasHeader-> expecting channel %d but found channel %d!\n", expected_channel, meas_header.channel );
		return false;
	}

	return true;
}

bool SiemensTool::CopyData( MRIMeasurement& meas, std::fstream& file_stream, int num_samples, int channel )
{
	file_stream.read( (char*)meas.GetData().GetDataIndex( 0, 0, channel, 0, 0, 0, 0, 0, 0, 0, 0 ), num_samples * 2 * sizeof( float ) );
	if( file_stream.fail() )
	{
		GIRLogger::LogError( "SiemensTool::CopyData -> failed to copy data for channel %d!\n", channel );
		return false;
	}

	return true;
}

void SiemensTool::UpdateDims( MRIDimensions& data_dims, MeasHeader& meas_header )
{
	data_dims.Line = std::max( data_dims.Line, meas_header.line+1 );
	data_dims.Set = std::max( data_dims.Set, meas_header.set+1 );
	data_dims.Phase = std::max( data_dims.Phase, meas_header.phase+1 );
	data_dims.Slice = std::max( data_dims.Slice, meas_header.slice+1 );

	data_dims.Echo = std::max( data_dims.Echo, meas_header.multi_echo+1 );
	data_dims.Repetition = std::max( data_dims.Repetition, meas_header.repeat+1 );
	//data_dims.Segment = std::max( data_dims.Segment, meas_header.segment+1 );
	data_dims.Partition = std::max( data_dims.Partition, meas_header.partition+1 );
	data_dims.Average = std::max( data_dims.Average, meas_header.average+1 );
}
