#include <ReconPlugin.h>
#include <GIRLogger.h>
#include <string>
#include <sstream>
#include <map>

ReconPlugin::ReconPlugin( const char* new_plugin_id, const char* new_alias ): plugin_id( new_plugin_id ), alias( new_alias ) {}

ReconPlugin::~ReconPlugin() {}
