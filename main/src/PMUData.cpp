#include <PMUData.h>
#include <GIRLogger.h>
#include <FilterTool.h>
//#include <Serializable.h>
#include <MRIDataComm.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <math.h>

#include <dirent.h>

PMUData::PMUData(): 
	mdh_start_time( -1 ),
	mdh_stop_time( -1 ),
	mpcu_start_time( -1 ),
	mpcu_stop_time( -1 ),
	sampling_period( -1.0 ),
	num_bins( -1 )
{}

bool PMUData::LoadFile( std::string pmu_path, int new_num_bins, bool minimal_load )
{
	num_bins = new_num_bins;

	// open the file
	std::ifstream pmu_file( pmu_path.c_str(), std::istream::in );
	if( !pmu_file.good() )
	{
		GIRLogger::LogError( "PMUData::LoadFile -> unable to open file: %s\n", pmu_path.c_str() );
		return false;
	}

	// parse values line
	std::string line;
	getline( pmu_file, line );
	if( !ParseWaveform( line ) )
	{
		GIRLogger::LogError( "PMUData::LoadFile -> ParseWaveForm failed for PMU file: %s\n", pmu_path.c_str() );
		return false;
	}

	while( !pmu_file.eof() && !pmu_file.fail() )
	{
		getline( pmu_file, line );
		if( !ParseNameValue( line ) )
		{
			GIRLogger::LogError( "PMUData::LoadFile -> ParseNameValue failed for PMU file: %s\n", pmu_path.c_str() );
			return false;
		}
	}
	pmu_file.close();

	// make sure we got everything
	if( mdh_start_time == -1 )
	{
		GIRLogger::LogError( "PMUData::LoadFile -> mdh_start_time never set for PMU file: %s\n", pmu_path.c_str() );
		return false;
	}
	if( mdh_stop_time == -1 )
	{
		GIRLogger::LogError( "PMUData::LoadFile -> mdh_stop_time never set for PMU file: %s\n", pmu_path.c_str() );
		return false;
	}
	if( mpcu_start_time == -1 )
	{
		GIRLogger::LogError( "PMUData::LoadFile -> mpcu_start_time never set for PMU file: %s\n", pmu_path.c_str() );
		return false;
	}
	if( mpcu_stop_time == -1 )
	{
		GIRLogger::LogError( "PMUData::LoadFile -> mpcu_stop_time never set for PMU file: %s\n", pmu_path.c_str() );
		return false;
	}

	// calculate sampling period 
	//!should just be 20 ms right?
	//sampling_period = (float)(mdh_stop_time - mdh_start_time) / values.size();
	sampling_period = 20;

	if( !minimal_load )
	{
		// find triggers
		if( !FindTriggers() )
		{
			GIRLogger::LogError( "PMUData::LoadFile -> FindTriggers failed for PMU file: %s\n", pmu_path.c_str() );
			return false;
		}
	
		// create bins
		if( !CreateBins() )
		{
			GIRLogger::LogError( "PMUData::LoadFile -> CreateBins failed for PMU file: %s\n", pmu_path.c_str() );
			return false;
		}
	}

	return true;
}

bool PMUData::ParseWaveform( std::string& line )
{
	int value;
	int value_count = 0;
	std::stringstream string_stream( line );
	while( string_stream >> value)
	{
		// skip the first 4 values
		if( ++value_count <= 4 )
			continue;
		// voltage
		if( value < 5000 )
		{
			//TODO: Jason deals with saturated periods, I'm not worrying about this right now
			values.push_back( value );
		}
		// trigger value
		else if( value < 6000 && values.size() > 0 )
			orig_triggers.push_back( values.size() - 1 );
		// 6000?
		else {}
	}

	return true;
}

bool PMUData::ParseNameValue( std::string& line )
{
	// end of file
	if( line.compare( "6003" ) == 0 )
		return true;

	// get colon index
	size_t colon_index = line.find_last_of( ":" );
	if( colon_index == line.npos )
	{
		GIRLogger::LogWarning( "PMUData::ParseNameValue -> no colon found in line, skipping line \"%s\"...\n", line.c_str() );
		return true;
	}

	// get name and value
	std::string name = line.substr( 0, colon_index );
	std::string value_string = line.substr( colon_index + 1 ); 
	std::stringstream stream( value_string );
	int value = 0;
	stream >> value;
	if( stream.fail() )
	{
		GIRLogger::LogError( "PMUData::ParseNameValue -> unable to parse value as integer: \"%s\"!\n", line.c_str() );
		return false;
	}

	// assign to member
	if( name.compare( "LogStartMDHTime" ) == 0 )
		mdh_start_time = value;
	else if( name.compare( "LogStopMDHTime" ) == 0 )
		mdh_stop_time = value;
	else if( name.compare( "LogStartMPCUTime" ) == 0 )
		mpcu_start_time = value;
	else if( name.compare( "LogStopMPCUTime" ) == 0 )
		mpcu_stop_time = value;

	return true;
}

bool PMUData::FindTriggers()
{
	// basically we smooth out the waveform and find zero crossings of the 1st derivative
	int num_items = values.size();
	float* values = new float[num_items];
	int i;
	for( i = 0; i < num_items; i++ )
		values[i] = values[i];

	float hann_kernel[21] = 
	{
		0.00000,
		0.02447,
		0.09549,
		0.20611,
		0.34549,
		0.50000,
		0.65451,
		0.79389,
		0.90451,
		0.97553,
		1.00000,
		0.97553,
		0.90451,
		0.79389,
		0.65451,
		0.50000,
		0.34549,
		0.20611,
		0.09549,
		0.02447,
		0.00000
	};

	// smooth out wave
	//NOTE: FilterTool::Conv2D currently shifts the output instead of assuming that the center of the kernel is at 0...
	//NOTE: so we have to shift the filtered data back by 10 later
	FilterTool::Conv2D( values, hann_kernel, values, 1, num_items, 1, 21, false, false );

	for( i = 10; i < num_items-1; i++ )
	{
		float diff_back = values[i] - values[i-1];
		float diff_forward = values[i+1] - values[i];

		// systole (1st derivative crosses zero from + to - [+ concavity] )
		if( diff_back >= 0 && diff_forward <=0 )
			systole_indicies.push_back(i-10);
		// diastole (1st derivative crosses zero from - to + [- concavity] )
		else if( diff_back <= 0 && diff_forward >=0 )
			diastole_indicies.push_back(i-10);
	}

	delete [] values;

	return true;
}

bool PMUData::CreateBins()
{
	int current_diastole = -1;

	for( int current_tick = 0; current_tick < (int)values.size(); current_tick++ )
	{
		int bin;

		if( current_tick == diastole_indicies[current_diastole+1] )
			current_diastole++;

		// can't bin before first diastole or after last diastole
		if( current_diastole == - 1 || current_diastole == (int)diastole_indicies.size() )
			bin = -1;
		else
		{
			int start_tick = diastole_indicies[current_diastole];
			int end_tick = diastole_indicies[current_diastole+1];

			int tick_duration = end_tick - start_tick;
			int rel_current_tick = current_tick - start_tick;

			//bin = ceil( num_bins * rel_current_tick / tick_duration ) + 1;
			bin = (int)ceil( num_bins * rel_current_tick / tick_duration );
		}

		bins.push_back( bin );
	}
	return true;
}

int PMUData::GetBin( int meas_time )
{
	int bin_index = (int)((meas_time - mdh_start_time) / sampling_period);
	if( bin_index >= (int)bins.size() || bin_index < 0 )
	{
		GIRLogger::LogError( "PMUData::GetBin -> requested a bin index that was out of range!  meas_time: %d, mdh_start_time: %d, sampling_period %d, bin_index %d\n", meas_time, mdh_start_time, sampling_period, bin_index );
		return -1;
	}
	return bins[bin_index];
}

std::string PMUData::ToString()
{
	std::stringstream stream;
	stream << "\tmdh_start_time: " << mdh_start_time << std::endl;
	stream << "\tmdh_stop_time: " << mdh_stop_time << std::endl;
	stream << "\tmpcu_start_time: " << mpcu_start_time << std::endl;
	stream << "\tmpcu_stop_time: " << mpcu_stop_time << std::endl;
	stream << "\tsampling_period: " << sampling_period << std::endl;
	stream << "\tnum_bins: " << num_bins << std::endl;

	return stream.str();
}

bool PMUData::GetPMUFile( std::vector<MRIMeasurement>& meas_vector, std::string& pmu_file, std::string& pmu_dir, std::string& pmu_prefix, std::string& pmu_id )
{
	// make sure we have at least one measurement
	if( meas_vector.size() == 0 )
		return false;

	// get min/max meas times
	int min_meas_time = meas_vector[0].meas_time;
	int max_meas_time = min_meas_time;
	for( std::vector<MRIMeasurement>::iterator it = meas_vector.begin(); it != meas_vector.end(); it++ )
	{
		min_meas_time = std::min( min_meas_time, it->meas_time );
		max_meas_time = std::max( max_meas_time, it->meas_time );
	}
	min_meas_time = (int)(2.5*min_meas_time);
	max_meas_time = (int)(2.5*max_meas_time);
	GIRLogger::LogDebug( "### min_time: %d, max_time: %d\n", min_meas_time, max_meas_time );

	// try indicated PMU file
	std::vector<std::string> files_to_check;
	files_to_check.push_back( pmu_dir + pmu_prefix + pmu_id + ".puls" );

	// try all pmu files in directory
	DIR* dir = opendir( pmu_dir.c_str() );
	if( dir == 0 )
	{
		GIRLogger::LogError( "PMUData::GetPMUFile -> could not open directory %s, aborting!", pmu_dir.c_str() );
		return false;
	}

	dirent *ent;
	GIRLogger::LogDebug( "### listing\n" );
	while( ( ent = readdir( dir ) ) != 0 )
	{
		std::string file_name = ent->d_name;
		GIRLogger::LogDebug( "### %s\n", file_name.c_str() );
		if( file_name.length() >= 5 && file_name.compare( file_name.length() - 5, 5, ".puls") == 0 )
			files_to_check.push_back( pmu_dir + file_name );
	}

	// check each pmu file
	for( std::vector<std::string>::iterator it = files_to_check.begin(); it != files_to_check.end(); it++ )
	{
		PMUData pmu_data;
		if( pmu_data.LoadFile( *it, 1, true ) )
		{
			//int pmu_ends = (int)(pmu_data.mdh_start_time + pmu_data.sampling_period * pmu_data.values.size());

			if( pmu_data.mdh_start_time <= min_meas_time && pmu_data.mdh_stop_time >= max_meas_time  )
			{
				GIRLogger::LogDebug( "### match!\n" );
				GIRLogger::LogDebug( "### begins %d\n", pmu_data.mdh_start_time );
				GIRLogger::LogDebug( "### ends   %d\n", (int)( pmu_data.mdh_start_time + pmu_data.sampling_period * pmu_data.values.size() ) );

				pmu_file = *it;
				return true;
			}
		}
	}

	return false;
}
