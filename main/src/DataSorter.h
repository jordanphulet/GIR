#ifndef DATA_SORTER_H
#define DATA_SORTER_H

#include <GIRConfig.h>
#include <vector>

class MRIMeasurement;
class MRIData;

class DataSorter
{
	public:
	DataSorter();
	bool Configure( GIRConfig& config, bool main_config, bool final_config );
	bool Sort( std::vector<MRIMeasurement>& meas_vector, MRIData& data );

	protected:
	std::string sort_method;
	std::string sort_script;
	std::string pmu_dir;
	std::string sort_script_dir;

	bool SortReplace( std::vector<MRIMeasurement>& meas_vector, MRIData& data );
};

#endif
