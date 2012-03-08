#ifndef GIR_UTILITIES_H
#define GIR_UTILITIES_H

#include <string>

class GIRUtils
{
	public:
	static void CompleteDirPath( std::string& path );
	static bool IsDir( const char* path );
	static bool FileIsReadable( const char* path );
};

#endif
