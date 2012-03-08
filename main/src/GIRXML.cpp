#include <GIRXML.h>
#include <GIRLogger.h>
#include <ReconPipeline.h>
#include <tinyxml/tinyxml.h>
#include <sstream>

TiXmlElement* GIRXML::GetBaseElement( TiXmlDocument& doc, const char* element_name )
{
	TiXmlElement* root = doc.RootElement();
	if( root == 0 || strcmp( root->Value(), "gir_xml" ) != 0 )
	{
		GIRLogger::LogError( "GIRXML::GetElement -> unable to get root element \"gir_xml\"!\n" );
		return 0;
	}
	return root->FirstChildElement( element_name );
}
bool GIRXML::Load( const char* xml_path, GIRConfig& config )
{
	// load xml document
	TiXmlDocument doc( xml_path );
	if( !doc.LoadFile() )
	{
		GIRLogger::LogError( "GIRXML::Load( GIRConfig ) -> unable to load xml file: \"%s,\" error: \"%s\"!\n", xml_path, doc.ErrorDesc() );
		return false;
	}

	// get config element
	TiXmlElement* config_element = GetBaseElement( doc, "config" );
	if( config_element == 0 )
	{
		GIRLogger::LogError( "GIRXML::Load( GIRConfig ) -> unable to find base element \"config\"!\n" );
		return false;
	}

	// loop through all the children of the config element
	TiXmlElement* config_child = config_element->FirstChildElement();
	while( config_child != 0 )
	{
		const char* child_name = config_child->Value();
		// param_set
		if( strcmp( child_name, "param_set" ) == 0 )
		{
			const char* att_plugin_id = config_child->Attribute( "plugin_id" );
			const char* att_plugin_alias = config_child->Attribute( "plugin_alias" );
			// loop through all the children of the param_set element
			TiXmlElement* set_child = config_child->FirstChildElement();
			while( set_child != 0 )
			{
				const char* set_child_name = set_child->Value();
				// param
				if( strcmp( set_child_name, "param" ) == 0 )
				{
					const char* att_name = set_child->Attribute( "name" );
					const char* att_value = set_child->Attribute( "value" );
					if( att_name != 0 && att_value != 0 )
						config.SetParam( att_plugin_id, att_plugin_alias, att_name, att_value );
					else
						GIRLogger::LogError( "GIRXML::Load( GIRConfig ) -> \"param\" must have both name and value attributes, param skipped!\n" );
						
				}
				else
					GIRLogger::LogWarning( "GIRXML::Load( GIRConfig ) -> unrecognized child of \"param_set\": \"%s!\"\n", set_child_name );
				// next child
				set_child = set_child->NextSiblingElement();
				
			}
		}
		// unrecognized tag
		else
			GIRLogger::LogWarning( "GIRXML::Load( GIRConfig ) -> unrecognized child of \"config\": \"%s!\"\n", child_name );
		// next child
		config_child = config_child->NextSiblingElement();
	}

	return true;
}

bool GIRXML::Load( const char* xml_path, ReconPipeline& pipeline, const char* plugin_dir )
{
	// load xml document
	TiXmlDocument doc( xml_path );
	if( !doc.LoadFile() )
	{
		GIRLogger::LogError( "GIRXML::Load( ReconPipeline ) -> unable to load xml file: \"%s,\" error: \"%s\"!\n", xml_path, doc.ErrorDesc() );
		return false;
	}


	// get pipeline element
	TiXmlElement* pipeline_element = GetBaseElement( doc, "pipeline" );
	if( pipeline_element == 0 )
	{
		GIRLogger::LogError( "GIRXML::Load( ReconPipeline ) -> unable to find base element \"pipeline\"!\n" );
		return false;
	}

	// loop through all the children, adding the plugins
	TiXmlElement* child = pipeline_element->FirstChildElement();
	while( child != 0 )
	{
		// process all plugin elements
		const char* child_name = child->Value();
		if( child_name != 0 && strcmp( child_name, "plugin" ) == 0 )
		{
			const char* att_id = child->Attribute( "id" );
			const char* att_alias = child->Attribute( "alias" );
			if( att_id != 0 && strlen( att_id ) != 0 && att_alias != 0 && strlen( att_alias ) != 0 )
			{
				// attempt to load the plugin
				stringstream plugin_path;
				plugin_path << plugin_dir << att_id << ".so";
				if( !pipeline.AddPlugin( plugin_path.str().c_str(), att_alias ) )
				{
					GIRLogger::LogError( "GIRXML::Load( ReconPipeline ) -> pipeline failed to load plugin: \"%s!\"\n", plugin_path.str().c_str() );
					return false;
				}
				GIRLogger::LogInfo( "plugin loaded: id=%s alias=%s\n", att_id, att_alias );
			}
			else
				GIRLogger::LogError( "GIRXML::Load( ReconPipeline ) -> \"plugin\" must have both an id and alias attribute, no plugin loaded!\n" );
		}
		child = child->NextSiblingElement();
	}

	// loop through all the children again, creating links
	child = pipeline_element->FirstChildElement();
	while( child != 0 )
	{
		// process all plugin elements
		const char* child_name = child->Value();
		if( child_name == 0 )
		{
			GIRLogger::LogWarning( "GIRXML::Load( ReconPipeline ) -> child had NULL name!\n", child_name );
			child = child->NextSiblingElement();
			continue;
		}

		// plugin, we've already processed this
		if( strcmp( child_name, "plugin" ) == 0 ) {}
		// root
		else if( strcmp( child_name, "root" ) == 0 )
		{
			// get alias attribute
			const char* att_alias = child->Attribute( "alias" );
			if( att_alias == 0 || strlen( att_alias ) == 0 )
				GIRLogger::LogWarning( "GIRXML::Load( ReconPipeline ) -> root without attribute \"alias\" detected!\n" );
			// set root
			else if( !pipeline.SetRoot( att_alias ) )
			{
				GIRLogger::LogError( "GIRXML::Load( ReconPipeline ) -> set root failed for alias: \"%s\"!\n", att_alias );
				return false;
			}
		}
		else if( strcmp( child_name, "link" ) == 0 )
		{
			// get input and output
			const char* att_input = child->Attribute( "input" );
			const char* att_output = child->Attribute( "output" );
			if( att_input == 0 || strlen( att_input ) == 0 || att_output == 0 || strlen( att_output ) == 0 )
				GIRLogger::LogError( "GIRXML::Load( ReconPipeline ) -> \"link\" must have both an input and output attribute, no link made!\n" );
			// link
			else if( !pipeline.Link( att_input, att_output ) )
			{
				GIRLogger::LogError( "GIRXML::Load( ReconPipeline ) -> link failed for input: \"%s\" and  output:\"%s\"!\n", att_input, att_output );
				return false;
			}
		}
		else
			GIRLogger::LogWarning( "GIRXML::Load( ReconPipeline ) -> unrecognized child of \"pipeline\": \"%s!\"\n", child_name );

		child = child->NextSiblingElement();
	}

	return true;
}
