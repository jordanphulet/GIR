#include <GIRUtils.h>
#include <sys/stat.h>
#include <string>
#include <fstream>

void GIRUtils::CompleteDirPath( std::string& path )
{
	if( path.size() > 0 && path.find_last_of("/") != path.size() - 1 )
		path += "/";
}

bool GIRUtils::IsDir( const char* path )
{
	struct stat st;
	return !( stat( path, &st ) != 0 || !S_ISDIR( st.st_mode ) );
}

bool GIRUtils::FileIsReadable( const char* path )
{
	std::fstream file_stream( path );
	bool readable = file_stream.good();
	file_stream.close();
	return readable;
}
