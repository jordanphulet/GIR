#include "GIRLogger.h"
#include <cstdarg>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <ctime>

GIRLogger* GIRLogger::instance = 0;

GIRLogger::GIRLogger(): silent( false ), log_to_file( false ), log_file( 0 )
{}

GIRLogger::~GIRLogger()
{
	if( log_file != 0 )
		fclose( log_file );
}

void GIRLogger::LogDebug( const char* message, ... )
{
	if( GIRLogger::Instance()->silent )
		return;

	FILE* print_file = ( Instance()->log_to_file )? Instance()->log_file: stdout;
	fprintf( print_file, "%s [DEBUG]: ", GetTimeStamp().c_str() );

	va_list arg_list;
	va_start( arg_list, message );
	vfprintf( print_file, message, arg_list );
	va_end( arg_list );
	fflush( print_file );
}

void GIRLogger::LogInfo( const char* message, ... )
{
	if( GIRLogger::Instance()->silent )
		return;

	FILE* print_file = ( Instance()->log_to_file )? Instance()->log_file: stdout;
	fprintf( print_file, "%s ", GetTimeStamp().c_str() );

	va_list arg_list;
	va_start( arg_list, message );
	vfprintf( print_file, message, arg_list );
	va_end( arg_list );
	fflush( print_file );
}

void GIRLogger::LogWarning( const char* message, ... )
{
	if( GIRLogger::Instance()->silent )
		return;

	FILE* print_file = ( Instance()->log_to_file )? Instance()->log_file: stdout;
	fprintf( print_file, "%s [WARNING]: ", GetTimeStamp().c_str() );

	va_list arg_list;
	va_start( arg_list, message );
	vfprintf( print_file, message, arg_list );
	va_end( arg_list );
	fflush( print_file );
}

void GIRLogger::LogError( const char* message, ... )
{
	if( GIRLogger::Instance()->silent )
		return;

	FILE* print_file = ( Instance()->log_to_file )? Instance()->log_file: stderr;
	fprintf( print_file, "%s [ERROR]: ", GetTimeStamp().c_str() );

	va_list arg_list;
	va_start( arg_list, message );
	vfprintf( print_file, message, arg_list );
	va_end( arg_list );
	fflush( print_file );
}

bool GIRLogger::LogToFile( const char* file_path )
{
	if( file_path == 0 )
	{
		fprintf( stderr, "GIRLogger::LogToFile -> file_path was NULL!\n" );
		return false;
	}

	if( log_file != 0 )
	{
		fclose( log_file );
		log_file = 0;
	}

	log_file = fopen( file_path, "a" );

	if( log_file == 0 )
	{
		fprintf( stderr, "GIRLogger::LogToFile -> can't open log file: %s\n", file_path );
		return false;
	}
	else
	{
		log_to_file = true;
		return true;
	}
}

GIRLogger* GIRLogger::Instance()
{
	if( GIRLogger::instance == 0 )
		GIRLogger::instance = new GIRLogger();
	return GIRLogger::instance;
}

void GIRLogger::DestoryInstance()
{
	if( GIRLogger::instance != 0 )
	{
		delete GIRLogger::instance;
		GIRLogger::instance = 0;
	}
}

std::string GIRLogger::GetTimeStamp()
{
	std::stringstream stream;

	time_t raw_time;
	struct tm* time_info;

	time( &raw_time );

	time_info = localtime( &raw_time );

	stream << std::setfill( '0' );
	stream << std::setw( 2 ) << time_info->tm_hour << ":";
	stream << std::setw( 2 ) << time_info->tm_min << ":";
	stream << std::setw( 2 ) << time_info->tm_sec << " ";
	stream << std::setw( 2 ) << time_info->tm_mon+1 << "/";
	stream << std::setw( 2 ) << time_info->tm_mday << "/";
	stream << std::setw( 2 ) << time_info->tm_year + 1900;

	return stream.str();
}
