#ifndef __RECON_PLUGIN_H__
#define __RECON_PLUGIN_H__

#include <MRIData.h>
#include <MRIDataComm.h>
#include <Serializable.h>
#include <GIRConfig.h>
#include <string>
#include <vector>
#include <map>

class ReconPlugin: public GIRConfigurable
{
	public:
	virtual ~ReconPlugin();

	const std::string& GetPluginID() { return plugin_id; }
	const std::string& GetAlias() { return alias; }

	virtual bool Configure( GIRConfig& config, bool main_config, bool final_config ) = 0;
	virtual bool Reconstruct( MRIData& mri_data ) = 0;
	virtual bool Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data ) = 0;
	virtual bool CanReconMeasData() { return false; }

	protected:
	const std::string plugin_id;
	const std::string alias;

	ReconPlugin( const char* plugin_id, const char* alias );
};

typedef ReconPlugin* plugin_create( const char* alias );
typedef void plugin_destroy( ReconPlugin* );

#endif
