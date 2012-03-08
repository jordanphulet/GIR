#ifndef RECON_PIPELINE_H
#define RECON_PIPELINE_H

#include <Serializable.h>
#include <ReconPlugin.h>
#include <GIRConfig.h>
#include <MRIDataComm.h>
#include <vector>

class MRIData;

class PluginProxy
{
	public:
	PluginProxy(): plugin( 0 ), next( 0 ), handle( 0 ), Create( 0 ), Destroy( 0 ) {}
	~PluginProxy();

	bool Load( std::string lib_path, std::string alias );

	ReconPlugin* plugin;
	PluginProxy* next;

	private:
	void* handle;
	plugin_create* Create;
	plugin_destroy* Destroy;
};

class ReconPipeline: public GIRConfigurable
{
	public:
	std::string sort_method;

	ReconPipeline(): root( 0 ) {}
	~ReconPipeline();

	bool AddPlugin( const char* plugin_path, const char* alias );
	bool Link( const char* source_alias, const char* sink_alias );
	bool SetRoot( const char* alias );

	bool Configure( GIRConfig& config, bool main_config, bool final_config );
	bool Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& data );

	private:
	std::map<std::string,PluginProxy*> plugins;
	PluginProxy* root;
};

#endif
