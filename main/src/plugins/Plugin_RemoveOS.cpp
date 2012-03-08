#include <Serializable.h>
#include <GIRLogger.h>
#include <GIRConfig.h>
#include <GIRUtils.h>
#include <plugins/Plugin_RemoveOS.h>
#include <map>
#include <sstream>

extern "C" ReconPlugin* create( const char* alias )
{
	return new Plugin_RemoveOS( "Plugin_RemoveOS", alias );
}

extern "C" void destroy( ReconPlugin* plugin )
{
	delete plugin;
}

bool Plugin_RemoveOS::Configure( GIRConfig& config, bool main_config, bool final_config )
{
	config.GetParam( plugin_id.c_str(), alias.c_str(), "os_x", os_x );
	config.GetParam( plugin_id.c_str(), alias.c_str(), "os_y", os_y );

	if( os_x <= 0 )
	{
		GIRLogger::LogError( "Plugin_RemoveOS::Configure -> os_x can't be less than or equal to 0!\n" );
		os_x = 1;
		return false;
	}

	if( os_y <= 0 )
	{
		GIRLogger::LogError( "Plugin_RemoveOS::Configure -> os_y can't be less than or equal to 0!\n" );
		os_y = 1;
		return false;
	}

	return true;
}

bool Plugin_RemoveOS::Reconstruct( MRIData& mri_data )
{
	using namespace std;
	bool succeeded = true;

	// start RemoveOS engine
	Engine* mat_engine = 0;
	if( !( mat_engine = engOpen( "\0" ) ) ) 
	{
		GIRLogger::LogError( "Plugin_RemoveOS::Reconstruct -> Can't start RemoveOS engine, reconstruction failed!\n" );
		return false;
	}

	// build matlab code line to get the matlab script
	std::stringstream script_stream;
	script_stream << "\tpath( '" << script_dir << "', path );\n";
	script_stream << "\tmatlab_script = str2func( '" << matlab_script << "' );\n";
	script_stream << "\tresult = single( matlab_script( k_space, input_struct ) );\n";

	//GIRLogger::LogDebug( "MATLAB COMMANDS:\n%s", script_stream.str().c_str() );
	GIRLogger::LogInfo( "Plugin_RemoveOS::Reconstruct -> executing matlab script: \"%s\"...\n", matlab_script.c_str() );

	// export data to mxArray
	mxArray* mx_data = MexData::ExportMexArray( mri_data );
	mxArray* input_struct = 0;
	mxArray* result = 0;

	try
	{
		// get field names
		const char* field_names [params.size()];
		int i = 0;
		map<string,string>::iterator it;
		for( it = params.begin(); it != params.end(); it++ )
		{
			field_names[i] = it->first.c_str();
			i++;
		}

		// create and fill structure
		mwSize struct_dims = 1;
		input_struct = mxCreateStructArray( struct_dims, &struct_dims, params.size(), field_names );
		i = 0;
		for( it = params.begin(); it != params.end(); it++ )
		{
			mxSetFieldByNumber( input_struct, 0, i, mxCreateString( it->second.c_str() ) );
			i++;
		}

		// put variables
		if( engPutVariable( mat_engine, "k_space", mx_data ) != 0 )
		{
			GIRLogger::LogError( "Plugin_RemoveOS::Reconstruct -> engPutVariable failed for k_space!\n", script_stream.str().c_str() );
			succeeded = false;
		}
		if( succeeded && engPutVariable( mat_engine, "input_struct", input_struct ) != 0 )
		{
			GIRLogger::LogError( "Plugin_RemoveOS::Reconstruct -> engPutVariable failed for input_struct!\n", script_stream.str().c_str() );
			succeeded = false;
		}

		// execute the script
		if( succeeded && engEvalString( mat_engine, script_stream.str().c_str() ) != 0 )
		{
			GIRLogger::LogError( "Plugin_RemoveOS::Reconstruct -> engEvalString failed!\n", script_stream.str().c_str() );
			succeeded = false;
		}

		// get result
		if( succeeded && ( result = engGetVariable( mat_engine ,"result" ) ) == 0 )
		{
			GIRLogger::LogError( "Plugin_RemoveOS::Reconstruct -> RemoveOS variable \"result\" never set, reconstruction failed!\n", script_stream.str().c_str() );
			succeeded = false;
		}
	}
	catch( ... )
	{
		mxDestroyArray( mx_data );
		if( input_struct != 0 )
			mxDestroyArray( input_struct );
		if( result != 0 )
			mxDestroyArray( result );
		if( mat_engine != 0 )
			engClose( mat_engine );
		GIRLogger::LogError( "Plugin_RemoveOS::Reconstruct -> RemoveOS engine threw an exception, reconstruction failed!\n" );
		return false;
		throw;
	}

	// put the data back into mri_data
	if( result != 0 )
		MexData::ImportMexArray( mri_data, result, true );

	// clean up
	mxDestroyArray( mx_data );
	if( input_struct != 0 )
		mxDestroyArray( input_struct );
	if( result != 0 )
		mxDestroyArray( result );
	if( mat_engine != 0 )
		engClose( mat_engine );

	return succeeded;
}
