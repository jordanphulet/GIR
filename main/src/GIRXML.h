#ifndef GIR_XML_H
#define GIR_XML_H

class ReconPipeline;
class GIRConfig;
class TiXmlElement;
class TiXmlDocument;

class GIRXML
{
	public:
	static TiXmlElement* GetBaseElement( TiXmlDocument& doc, const char* element );
	static bool Load( const char* xml_path, GIRConfig& config );
	static bool Load( const char* xml_path, ReconPipeline& pipeline, const char* pipeline_dir );
};

#endif
