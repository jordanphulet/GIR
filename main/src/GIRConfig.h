#ifndef GIR_CONFIG_H
#define GIR_CONFIG_H

#include "Serializable.h"
#include <string>
#include <map>

using namespace std;

class GIRConfig: public MRISerializable
{

	public:
	GIRConfig();
	~GIRConfig();

	bool GetParam( const char* plugin_id, const char* plugin_alias, const char* param_name, string& dest );
	bool GetParam( const char* plugin_id, const char* plugin_alias, const char* param_name, float& dest );
	bool GetParam( const char* plugin_id, const char* plugin_alias, const char* param_name, double& dest );
	bool GetParam( const char* plugin_id, const char* plugin_alias, const char* param_name, int& dest );
	bool GetParam( const char* plugin_id, const char* plugin_alias, const char* param_name, bool& dest );

	bool SetParam( const char* plugin_id, const char* plugin_alias, const char* param_name, const char* value );

	void LoadParams( const char* plugin_id, const char* plugin_alias, map<string,string>& params );

	void Add( GIRConfig& config );

	std::string ToString();

	int Serialize( char* buffer, int buffer_size );
	int Unserialize( char* buffer, int buffer_size );

	private:
	map<string,string> main_params;
	map<string,map<string,string> > plugin_id_params;
	map<string,map<string,string> > plugin_alias_params;

	bool SerializeStringStringMap( std::map<std::string,std::string>& ss_map, char*& buffer, int& buffer_size, int& ser_size );
	bool UnserializeStringStringMap( std::map<std::string,std::string>& ss_map, char*& buffer, int& buffer_size, int& ser_size );
};

class GIRConfigurable
{
	public:
	virtual ~GIRConfigurable() {}

	virtual bool Configure( GIRConfig& config, bool main_config, bool final_config ) = 0;
};

#endif
