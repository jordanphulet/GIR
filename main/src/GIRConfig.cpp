#include "GIRConfig.h"
#include "GIRLogger.h"
#include <map>
#include <string>
#include <cstring>
#include <sstream>
#include <set>

GIRConfig::GIRConfig()
{}

GIRConfig::~GIRConfig()
{}

bool GIRConfig::GetParam( const char* plugin_id, const char* plugin_alias, const char* param_name, string& dest )
{
	bool found_param = false;
	map<string,map<string,string> >::iterator it1;
	map<string,string>::iterator it2;

	// main parameter
	if( ( plugin_id == 0 || strlen( plugin_id ) == 0 ) && ( plugin_alias == 0 || strlen( plugin_alias ) == 0 ) )
	{
		if(	( it2 = main_params.find( param_name ) ) != main_params.end() )
		{
			dest = it2->second;
			found_param = true;
		}
	}
	// plugin parameter
	else
	{
		// check plugin id
		if(	plugin_id != 0 )
		{
			//GIRLogger::LogDebug( "### looking for parameter: %s in ids for %s\n", param_name, plugin_id );
			if(
				( it1 = plugin_id_params.find( plugin_id ) ) != plugin_id_params.end() &&
				( it2 = it1->second.find( param_name ) ) != it1->second.end() )
			{
				//GIRLogger::LogDebug( "##### found it: %s\n", it2->second.c_str() );
				dest = it2->second;
				found_param = true;
			}
		}
		// check plugin alias
		if(	plugin_alias != 0 )
		{
			//GIRLogger::LogDebug( "### looking for parameter: %s in aliases for %s\n", param_name, plugin_alias );
			if(
				( it1 = plugin_alias_params.find( plugin_alias ) ) != plugin_alias_params.end() &&
				( it2 = it1->second.find( param_name ) ) != it1->second.end() )
			{
				//GIRLogger::LogDebug( "##### found it: %s\n", it2->second.c_str() );
				dest = it2->second;
				found_param = true;
			}
		}
	}

	return found_param;
}

bool GIRConfig::GetParam( const char* plugin_id, const char* plugin_alias, const char* param_name, float& dest )
{
	// get value string 
	string param_value;
	if( !GetParam( plugin_id, plugin_alias, param_name, param_value ) )
		return false;

	// convert to float
	stringstream stream( param_value );
	float value = 0;
	stream >> value;
	if( stream.fail() )
	{
		GIRLogger::LogError( "GIRConfig::GetParam -> could not convert to float: \"%s\", parameter not loaded...\n", param_value.c_str() );
		return false;
	}

	dest = value;
	return true;
}

bool GIRConfig::GetParam( const char* plugin_id, const char* plugin_alias, const char* param_name, double& dest )
{
	// get value string 
	string param_value;
	if( !GetParam( plugin_id, plugin_alias, param_name, param_value ) )
		return false;

	// convert to double
	stringstream stream( param_value );
	double value = 0;
	stream >> value;
	if( stream.fail() )
	{
		GIRLogger::LogError( "GIRConfig::GetParam -> could not convert to double: \"%s\", parameter not loaded...\n", param_value.c_str() );
		return false;
	}

	dest = value;
	return true;
}

bool GIRConfig::GetParam( const char* plugin_id, const char* plugin_alias, const char* param_name, int& dest )
{
	// get value string 
	string param_value;
	if( !GetParam( plugin_id, plugin_alias, param_name, param_value ) )
		return false;

	// convert to int 
	stringstream stream( param_value );
	int value = 0;
	stream >> value;
	if( stream.fail() )
	{
		GIRLogger::LogError( "GIRConfig::GetParam -> could not convert to int: \"%s\", parameter not loaded...\n", param_value.c_str() );
		return false;
	}

	dest = value;
	return true;
}

bool GIRConfig::GetParam( const char* plugin_id, const char* plugin_alias, const char* param_name, bool& dest )
{
	// get value string 
	string param_value;
	if( !GetParam( plugin_id, plugin_alias, param_name, param_value ) )
		return false;

	// convert to bool
	if( param_value.compare( "true" ) == 0 || param_value.compare( "True" ) == 0 || param_value.compare( "TRUE" ) == 0 )
	{
		dest = true;
		return true;
	}
	if( param_value.compare( "false" ) == 0 || param_value.compare( "False" ) == 0 || param_value.compare( "FALSE" ) == 0 )
	{
		dest = false;
		return true;
	}

	GIRLogger::LogError( "GIRConfig::GetParam -> could not convert to bool: \"%s\", parameter not loaded...\n", param_value.c_str() );
	return false;
}

bool GIRConfig::SetParam( const char* plugin_id, const char* plugin_alias, const char* param_name, const char* value )
{
	// make sure a name and value are specified
	if( param_name == 0 || strlen( param_name ) == 0 || value == 0 )
	{
		GIRLogger::LogError( "GIRConfig::SetParam -> either param_name or value was NULL!\n" );
		return false;
	}

	// if an alias is specified put it in the alias map
	if( plugin_alias != 0 && strlen( plugin_alias ) != 0 )
	{
		//GIRLogger::LogDebug( "setting alias param: %s::%s -> %s\n", plugin_alias, param_name, value );
		std::map<string,string>& alias_map = plugin_alias_params[plugin_alias];
		alias_map.insert( std::pair<std::string,std::string>( param_name, value ) );
	}
	// if an id is specified put it in the id map
	else if( plugin_id != 0 && strlen( plugin_id ) != 0 )
	{
		//GIRLogger::LogDebug( "setting id param: %s::%s -> %s\n", plugin_id, param_name, value );
		std::map<string,string>& id_map = plugin_id_params[plugin_id];
		id_map.insert( std::pair<std::string,std::string>( param_name, value ) );
	}
	// otherwise put it in the main map
	else
		//GIRLogger::LogDebug( "setting main param: %s -> %s\n", param_name, value );
		main_params.insert( std::pair<std::string,std::string>( param_name, value ) );

	return true;
}

void GIRConfig::LoadParams( const char* plugin_id, const char* plugin_alias, std::map<std::string,std::string>& params )
{
	std::set<std::string> param_set;

	// get main params
	std::map<std::string,std::string>::iterator it1;
	for( it1 = main_params.begin(); it1 != main_params.end(); it1++ )
		param_set.insert( it1->first );

	// get plugin id params
	std::map<std::string,std::map<std::string,std::string> >::iterator it2;
	it2 = plugin_id_params.find( plugin_id );
	if( it2 != plugin_id_params.end() )
		for( it1 = it2->second.begin(); it1 != it2->second.end(); it1++ )
			param_set.insert( it1->first );

	// get plugin alias params
	it2 = plugin_alias_params.find( plugin_alias );
	if( it2 != plugin_alias_params.end() )
		for( it1 = it2->second.begin(); it1 != it2->second.end(); it1++ )
			param_set.insert( it1->first );

	// try to add them all
	std::string param_value;
	std::set<std::string>::iterator it3;
	for( it3 = param_set.begin(); it3 != param_set.end(); it3++ )
		if( GetParam( plugin_id, plugin_alias, it3->c_str(), param_value ) )
			params.insert( std::pair<std::string,std::string>( *it3, param_value ) );
}

void GIRConfig::Add( GIRConfig& config )
{
	main_params.insert( config.main_params.begin(), config.main_params.end() );
	plugin_alias_params.insert( config.plugin_alias_params.begin(), config.plugin_alias_params.end() );
	plugin_id_params.insert( config.plugin_id_params.begin(), config.plugin_id_params.end() );
}

std::string GIRConfig::ToString()
{
	stringstream stream;
	std::map<std::string,std::string>::iterator it1;
	std::map<std::string,std::map<std::string,std::string> >::iterator it2;

	// main
	stream << "main_params:\n";
	for( it1 = main_params.begin(); it1 != main_params.end(); it1++ )
		stream << "\t" << it1->first << " -> " << it1->second << "\n";
	// plugin_id 
	stream << "plugin_id_params:\n";
	for( it2 = plugin_id_params.begin(); it2 != plugin_id_params.end(); it2++ )
		for( it1 = it2->second.begin(); it1 != it2->second.end(); it1++ )
			stream << "\t" << it2->first << "::" << it1->first << " -> " << it1->second << "\n";
	// plugin_alias 
	stream << "plugin_alias_params:\n";
	for( it2 = plugin_alias_params.begin(); it2 != plugin_alias_params.end(); it2++ )
		for( it1 = it2->second.begin(); it1 != it2->second.end(); it1++ )
			stream << "\t" << it2->first << "::" << it1->first << " -> " << it1->second << "\n";

	return stream.str();
}

int GIRConfig::Serialize( char* buffer, int buffer_size )
{
	int ser_size = 0;


	// serialize main params
	if( !SerializeStringStringMap( main_params, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "GIRConfig::Serialize-> couldn't serialize main_params, serialization failed!\n" );
		return -1;
	}

	// serialize number of id param maps
	if( !SerializeInt( plugin_id_params.size(), buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "GIRConfig::Serialize-> couldn't serialize number of id param maps, serialization failed!\n" );
		return -1;
	}

	// serialize id param maps
	std::map<std::string,std::map<std::string,std::string> >::iterator it;
	for( it = plugin_id_params.begin(); it != plugin_id_params.end(); it++ )
	{
		//serialize key
		if( !SerializeString( it->first, buffer, buffer_size, ser_size ) )
		{
			GIRLogger::LogError( "GIRConfig::Serialize-> couldn't serialize id param map key, serialization failed!\n" );
			return -1;
		}
		// serialize value
		if( !SerializeStringStringMap( it->second, buffer, buffer_size, ser_size ) )
		{
			GIRLogger::LogError( "GIRConfig::Serialize-> couldn't serialize id param map, serialization failed!\n" );
			return -1;
		}
	}

	// serialize number of alias param maps
	if( !SerializeInt( plugin_alias_params.size(), buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "GIRConfig::Serialize-> couldn't serialize number of alias param maps, serialization failed!\n" );
		return -1;
	}

	// serialize alias param maps
	for( it = plugin_alias_params.begin(); it != plugin_alias_params.end(); it++ )
	{
		//serialize key
		if( !SerializeString( it->first, buffer, buffer_size, ser_size ) )
		{
			GIRLogger::LogError( "GIRConfig::Serialize-> couldn't serialize alias param map key, serialization failed!\n" );
			return -1;
		}
		// serialize value
		if( !SerializeStringStringMap( it->second, buffer, buffer_size, ser_size ) )
		{
			GIRLogger::LogError( "GIRConfig::Serialize-> couldn't serialize alias param map, serialization failed!\n" );
			return -1;
		}
	}

	return ser_size;
}

int GIRConfig::Unserialize( char* buffer, int buffer_size )
{
	int ser_size = 0;

	// unserialize main params
	if( !UnserializeStringStringMap( main_params, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "GIRConfig::Unserialize-> couldn't unserialize main_params, serialization failed!\n" );
		return -1;
	}

	// unserialize number of id param maps
	int id_params_size;
	if( !UnserializeInt( id_params_size, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "GIRConfig::Unserialize-> couldn't unserialize number of id param maps, serialization failed!\n" );
		return -1;
	}

	// unserialize id param maps
	int i;
	for( i = 0; i < id_params_size; i++ )
	{
		std::pair<std::string,std::map<std::string,std::string> > new_pair;
		//serialize key
		if( !UnserializeString( new_pair.first, buffer, buffer_size, ser_size ) )
		{
			GIRLogger::LogError( "GIRConfig::Unserialize-> couldn't unserialize id param map key, serialization failed!\n" );
			return -1;
		}
		// unserialize value
		if( !UnserializeStringStringMap( new_pair.second, buffer, buffer_size, ser_size ) )
		{
			GIRLogger::LogError( "GIRConfig::Unserialize-> couldn't unserialize id param map, serialization failed!\n" );
			return -1;
		}
		plugin_id_params.insert( new_pair );
	}

	// unserialize number of alias param maps
	int alias_params_size;
	if( !UnserializeInt( alias_params_size, buffer, buffer_size, ser_size ) )
	{
		GIRLogger::LogError( "GIRConfig::Unserialize-> couldn't unserialize number of alias param maps, serialization failed!\n" );
		return -1;
	}

	// unserialize alias param maps
	for( i = 0; i < alias_params_size; i++ )
	{
		std::pair<std::string,std::map<std::string,std::string> > new_pair;
		//serialize key
		if( !UnserializeString( new_pair.first, buffer, buffer_size, ser_size ) )
		{
			GIRLogger::LogError( "GIRConfig::Unserialize-> couldn't unserialize alias param map key, serialization failed!\n" );
			return -1;
		}
		// unserialize value
		if( !UnserializeStringStringMap( new_pair.second, buffer, buffer_size, ser_size ) )
		{
			GIRLogger::LogError( "GIRConfig::Unserialize-> couldn't unserialize alias param map, serialization failed!\n" );
			return -1;
		}
		plugin_alias_params.insert( new_pair );
	}

	return ser_size;
}

bool GIRConfig::SerializeStringStringMap( std::map<std::string,std::string>& ss_map, char*& buffer, int& buffer_size, int& ser_size )
{
	// serialize number of params
	if( !SerializeInt( ss_map.size(), buffer, buffer_size, ser_size ) )
		return false;

	std::map<std::string,std::string>::iterator it;
	for( it = ss_map.begin(); it != ss_map.end(); it++ )
	{
		//serialize key
		if( !SerializeString( it->first, buffer, buffer_size, ser_size ) )
			return false;
		// serialize value
		if( !SerializeString( it->second, buffer, buffer_size, ser_size ) )
			return false;
	}
	return true;
}

bool GIRConfig::UnserializeStringStringMap( std::map<std::string,std::string>& ss_map, char*& buffer, int& buffer_size, int& ser_size )
{
	// unserialize number of params
	int num_params;
	if( !UnserializeInt( num_params, buffer, buffer_size, ser_size ) )
		return false;

	for( int i = 0; i < num_params; i++ )
	{
		std::pair<std::string,std::string> new_pair;
		// unserialize key
		if( !UnserializeString( new_pair.first, buffer, buffer_size, ser_size ) )
			return false;
		// unserialize value
		if( !UnserializeString( new_pair.second, buffer, buffer_size, ser_size ) )
			return false;
		ss_map.insert( new_pair );
	}
	return true;
}
