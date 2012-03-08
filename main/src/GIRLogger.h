#ifndef GIR_LOGGER_H
#define GIR_LOGGER_H

#include <cstdio>
#include <string>

class GIRLogger
{
	public:
	enum GIRLogLevel { GIR_LOG_DEBUG, GIR_LOG_INFO, GIR_LOG_WARNING, GIR_LOG_ERROR };
	bool silent;

	~GIRLogger();

	//void Log( GIRLogLevel log_level, const char* message, ... );
	bool LogToFile( const char* file_path );

	static void LogDebug( const char* message, ... );
	static void LogInfo( const char* message, ... );
	static void LogWarning( const char* message, ... );
	static void LogError( const char* message, ... );

	static GIRLogger* Instance();
	static void DestoryInstance();

	private:
	static GIRLogger* instance;
	bool log_to_file;
	FILE* log_file;

	GIRLogger();
	static std::string GetTimeStamp();
};

#endif
