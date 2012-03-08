#ifndef PMUDATA_H
#define PMUDATA_H

#include <vector>
#include <string>

class MRIMeasurement;

class PMUData 
{
	public:
	// times are stored in milliseconds
	int mdh_start_time;
	int mdh_stop_time;
	int mpcu_start_time;
	int mpcu_stop_time;
	float sampling_period;
	int num_bins;
	std::vector<int> values;
	std::vector<int> orig_triggers;
	std::vector<int> triggers;
	std::vector<int> diastole_indicies;
	std::vector<int> systole_indicies;
	std::vector<int> bins;

	PMUData();
	
	bool LoadFile( std::string pmu_path, int num_bins, bool minimal_load = false );
	bool ParseWaveform( std::string& line );
	bool ParseNameValue( std::string& line );
	int GetBin( int meas_time );

	std::string ToString();

	static bool GetPMUFile( std::vector<MRIMeasurement>& meas_vector, std::string& pmu_file, std::string& pmu_dir, std::string& pmu_prefix, std::string& pmu_id );

	private:
	int ParseTime( std::string &line );
	bool FindTriggers(); //NOTE: this only really works for pulse-ox data...
	bool CreateBins();

};

#endif
