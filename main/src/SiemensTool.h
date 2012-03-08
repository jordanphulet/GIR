#ifndef SIEMENS_TOOL_H
#define SIEMENS_TOOL_H

#include <vector>
#include <fstream>
#include <stdint.h>

class MRIDataHeader;
class MRIMeasurement;
class MRIDimensions;
struct MeasHeader;

class SiemensTool
{
	public:
	static bool LoadDatFile( std::string dat_file_path, MRIDataHeader& header, std::vector<MRIMeasurement>& meas_vector );

	private:
	static bool ReadMeas( std::fstream& file_stream, MRIDimensions& data_dims, std::vector<MRIMeasurement>& meas_vector );
	static bool GetMeasHeader( std::fstream& file_stream, MeasHeader& meas_header, int expected_channel );
	static bool CopyData( MRIMeasurement& meas, std::fstream& file_stream, int num_samples, int channel );
	static void UpdateDims( MRIDimensions& data_dims, MeasHeader& meas_header );
};

struct MeasHeader
{
	uint32_t dma;
	int32_t id;
	uint32_t scan;
	uint32_t time;
	uint32_t pmu_time;
	uint32_t mask_1;
	uint32_t mask_2;
	uint16_t samples;
	uint16_t channels;
	uint16_t line;
	uint16_t average;
	uint16_t slice;
	uint16_t partition;
	uint16_t multi_echo;
	uint16_t phase;
	uint16_t repeat;
	uint16_t set;
	uint16_t echo;
	uint16_t dim_a;
	uint16_t dim_b;
	uint16_t dim_c;
	uint16_t dim_e;
	uint16_t dim_f;
	uint16_t pre_zero;
	uint16_t post_zero;
	uint16_t k_center;
	uint16_t coil;
	float roffset;
	uint32_t rftime;
	uint16_t line_center;
	uint16_t partition_center;
	uint16_t ice_param_1;
	uint16_t ice_param_2;
	uint16_t ice_param_3;
	uint16_t ice_param_4;
	uint16_t ice_param_5;
	uint16_t ice_param_6;
	uint16_t ice_param_7;
	uint16_t ice_param_8;
	float sag;
	float cor;
	float tra;
	float rotate_1;
	float rotate_2;
	float rotate_3;
	float rotate_4;
	uint16_t channel;
	uint16_t tab;
};

#endif
