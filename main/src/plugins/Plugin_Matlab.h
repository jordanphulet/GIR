#ifndef PLUGIN_MATLAB_H
#define PLUGIN_MATLAB_H

#include <ReconPlugin.h>
#include <Serializable.h>
#include <mex.h>

class GIRConfig;

class Plugin_Matlab: public ReconPlugin
{
	public:
	Plugin_Matlab( const char* new_plugin_id, const char* new_alias ): ReconPlugin( new_plugin_id, new_alias ) {}

	protected:
	std::string script_dir;
	std::string matlab_script;
	std::map<std::string,std::string> params;

	void LoadParams();
	bool Reconstruct( MRIData& mri_data );
	bool Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data );
	bool Configure( GIRConfig& config, bool main_config, bool final_config );

	void CreateParamsStructure( mxArray** input_struct );
	void CreateMeasStructure( std::vector<MRIMeasurement>& meas_vector, mxArray** meas_struct );

	virtual bool CanReconMeasData() { return true; }
};

#endif
