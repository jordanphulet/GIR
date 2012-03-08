#include <ReconPipeline.h>
#include <GIRLogger.h>
#include <GIRConfig.h>
#include <GIRUtils.h>
#include <Serializable.h>
#include <tinyxml/tinyxml.h>
#include <dlfcn.h>
#include <set>

PluginProxy::~PluginProxy()
{
	// delete the plugin
	if( plugin != 0 ) 
	{
		if( Destroy == 0 )
			GIRLogger::LogError( "PluginProxy::~PluginProxy -> Destroy() == 0, plugin not destroyed!\n" );
		else
			Destroy( plugin );
	}

	// unload the .so
	if( handle != 0 )
		dlclose( handle );

	handle = 0;
	plugin = 0;
	Create = 0;
	Destroy = 0;
}

bool PluginProxy::Load( std::string lib_path, std::string alias )
{
	// load the library
	void* plugin_lib = dlopen( lib_path.c_str(), RTLD_NOW | RTLD_GLOBAL );
	if( !plugin_lib )
	{
		GIRLogger::LogError( "PluginProxy::Load -> Unable to load plugin \"%s\", error: \"%s\"!\n", lib_path.c_str(), dlerror() );
		return false;
	}

	// reset errors
	dlerror();
    
	// load create symbol
	Create = (plugin_create*) dlsym( plugin_lib, "create" );
	const char* dlsym_error = dlerror();
	if( dlsym_error )
	{
		GIRLogger::LogError( "PluginProxy::Load -> can't load symbol \"create\", error: \"%s\"!\n", dlsym_error );
		return false;
	}

	// load destroy symbol
	Destroy = (plugin_destroy*) dlsym( plugin_lib, "destroy" );
	dlsym_error = dlerror();
	if( dlsym_error )
	{
		GIRLogger::LogError( "PluginProxy::Load -> can't load symbol \"destroy\", error: \"%s\"!\n", dlsym_error );
		return false;
	}

	// create object
	plugin = Create( alias.c_str() );

	return true;
}

ReconPipeline::~ReconPipeline()
{
	// delete all the plugins
	std::map<std::string,PluginProxy*>::iterator it;
	for( it = plugins.begin(); it != plugins.end(); it++ )
		if( it->second != 0 )
			delete it->second;
}

bool ReconPipeline::AddPlugin( const char* lib_path, const char* alias )
{
	// delete the plugin if it already exists
	std::map<std::string,PluginProxy*>::iterator it = plugins.find( alias );
	if( it != plugins.end() && it->second != 0 )
		delete it->second;

	// load the plugin and add it to the plugins map
	PluginProxy* new_plugin = 0;
	try 
	{
		new_plugin = new PluginProxy();
		if( new_plugin->Load( lib_path, alias ) )
		{
			plugins.insert( std::pair<std::string,PluginProxy*>( alias, new_plugin ) );
			return true;
		}
		else
		{
			GIRLogger::LogError( "ReconPipeline::AddPlugin -> load plugin failed for library: %s!\n", lib_path );
			delete new_plugin;
			return false;
		}
	}
	catch( ... )
	{
		// clean up if an exception was thrown
		GIRLogger::LogError( "ReconPipeline::AddPlugin -> an exception was thrown while tyring to load plugin: %s!\n", lib_path );
		if( new_plugin != 0 )
			delete new_plugin;
		return false;
	}
}

bool ReconPipeline::Link( const char* source_alias, const char* sink_alias )
{
	PluginProxy* source = 0;
	PluginProxy* sink = 0;

	// find source
	std::map<std::string,PluginProxy*>::iterator it = plugins.find( source_alias );
	if( it != plugins.end() )
		source = it->second;
	else
	{
		GIRLogger::LogError( "ReconPipeline::Link -> source plugin: %s was not found in loaded plugins!\n", source_alias );
		return false;
	}

	// find sink
	it = plugins.find( sink_alias );
	if( it != plugins.end() )
		sink = it->second;
	else
	{
		GIRLogger::LogError( "ReconPipeline::Link -> sink plugin: %s was not found in loaded plugins!\n", sink_alias );
		return false;
	}

	// make link
	if( source != 0 && sink != 0 )
	{
		source->next = sink;
		return true;
	}
	else
	{
		GIRLogger::LogError( "ReconPipeline::Link -> either source: %s or sink: %s was null!\n", source_alias, sink_alias );
		return false;
	}
}

bool ReconPipeline::SetRoot( const char* alias )
{
	if( alias == 0 )
	{
		GIRLogger::LogError( "ReconPipeline::SetRoot -> alias was null!\n", alias );
		return false;
	}
	std::map<std::string,PluginProxy*>::iterator it = plugins.find( alias );
	if( it != plugins.end() )
	{
		root = it->second;
		return true;
	}
	else
	{
		GIRLogger::LogError( "ReconPipeline::SetRoot -> plugin: %s was not found in loaded plugins!\n", alias );
		return false;
	}
	return false;
}

bool ReconPipeline::Configure( GIRConfig& config, bool main_config, bool final_config )
{
	bool success = true;

	// configure all the plugins
	std::map<std::string,PluginProxy*>::iterator it;
	for( it = plugins.begin(); it != plugins.end(); it++ )
	{
		if( it->second == 0 || it->second->plugin == 0 )
		{
			GIRLogger::LogError( "ReconPipeline::Confgiure -> NULL plugin found in plugins!\n" );
			success = false;
		}
		else
		{
			if( !it->second->plugin->Configure( config, main_config, final_config ) )
				success = false;
		}
	}

	return success;
}

bool ReconPipeline::Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& data )
{
	bool success = true;

	// make sure we have at least one plugin
	if( root == 0 || root->plugin == 0 )
	{
		GIRLogger::LogError( "ReconPipeline::Reconstruct -> root is NULL!\n" );
		return false;
	}

	// root must be able to reconstruct measurement data
	if( !root->plugin->CanReconMeasData() )
	{
		GIRLogger::LogError( "ReconPipeline::Reconstruct -> specified root plugin cannot reconstruct meas data!\n" );
		return false;
	}

	// reconstruct root
	if( !root->plugin->Reconstruct( meas_vector, data ) )
	{
		GIRLogger::LogError( "ReconPipeline::Reconstruct -> recon for root plugin failed!\n" );
		return false;
	}

	// iterate through the pipeline
	std::set<std::string> visited_aliases;
	PluginProxy* current_plugin = root->next;
	while( current_plugin != 0 && success )
	{
		// make sure each proxy has a valid plugin
		if( current_plugin->plugin == 0 )
		{
			GIRLogger::LogError( "ReconPipeline::Reconstruct -> PluginProxy with NULL plugin exists in pipeline!\n" );
			return false;
		}

		// make sure there aren't any cycles
		std::string alias = current_plugin->plugin->GetAlias();
		std::set<std::string>::iterator it = visited_aliases.find( alias );
		if( it != visited_aliases.end() )
		{
			GIRLogger::LogError( "ReconPipeline::Reconstruct -> cycle detected in pipeline at \"%s!\"\n", alias.c_str() );
			return false;
		}
		visited_aliases.insert( alias );

		//reconstruct
		success = success && current_plugin->plugin->Reconstruct( data );

		current_plugin = current_plugin->next;
	}

	return success;
}
