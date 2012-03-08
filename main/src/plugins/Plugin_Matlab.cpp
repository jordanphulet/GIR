#include <Serializable.h>
#include <GIRLogger.h>
#include <GIRConfig.h>
#include <GIRUtils.h>
#include <plugins/Plugin_Matlab.h>
#include <matlab/MexData.h>
#include <mex.h>
#include <engine.h>
#include <map>
#include <sstream>

extern "C" ReconPlugin* create( const char* alias )
{
	return new Plugin_Matlab( "Plugin_Matlab", alias );
}

extern "C" void destroy( ReconPlugin* plugin )
{
	delete plugin;
}

bool Plugin_Matlab::Configure( GIRConfig& config, bool main_config, bool final_config )
{
	if( main_config )
	{
		// get script directory
		if( config.GetParam( plugin_id.c_str(), alias.c_str(), "script_dir", script_dir ) )
			GIRUtils::CompleteDirPath( script_dir );
	}
	else
	{
		// script_dir can only be set in the main config
		std::string desired_script_dir;
		if( config.GetParam( plugin_id.c_str(), alias.c_str(), "script_dir", desired_script_dir ) )
		{
			GIRLogger::LogError( "Plugin_Matlab::Configure -> script_dir can only by set in the main config!\n" );
			return false;
		}
	}

	// get script
	if( config.GetParam( plugin_id.c_str(), alias.c_str(), "matlab_script", matlab_script ) )
	{
		// relative paths are not allowed
		if( matlab_script.find_last_of( "/" ) != matlab_script.npos )
		{
			GIRLogger::LogError( "Plugin_Matlab::Configure -> invalid character '/' in matlab_script!\n" );
			return false;
		}
	}

	// make sure we can read the script
	string full_script_path = script_dir + matlab_script + ".m";
	if( final_config && !GIRUtils::FileIsReadable( full_script_path.c_str() ) )
	{
		GIRLogger::LogError( "Plugin_Matlab::Configure -> specified script: %s could not be read!\n", full_script_path.c_str() );
		return false;
	}

	// get the parameters
	config.LoadParams( plugin_id.c_str(), alias.c_str(), params);

	return true;
}

bool Plugin_Matlab::Reconstruct( std::vector<MRIMeasurement>& meas_vector, MRIData& mri_data )
{
	using namespace std;
	bool succeeded = true;

	// start Matlab engine
	Engine* mat_engine = 0;
	if( !( mat_engine = engOpen( "\0" ) ) ) 
	{
		GIRLogger::LogError( "Plugin_Matlab::Reconstruct -> Can't start Matlab engine, reconstruction failed!\n" );
		return false;
	}

	// build matlab code line to get the matlab script
	std::stringstream script_stream;
	script_stream << "\tpath( '" << script_dir << "', path );\n";
	script_stream << "\tmatlab_script = str2func( '" << matlab_script << "' );\n";
	script_stream << "\tresult = single( matlab_script( k_space, input_struct, meas_struct ) );\n";

	//GIRLogger::LogDebug( "MATLAB COMMANDS:\n%s", script_stream.str().c_str() );
	GIRLogger::LogInfo( "Plugin_Matlab::Reconstruct -> executing matlab script: \"%s\"...\n", matlab_script.c_str() );

	// export data to mxArray
	mxArray* mx_data = MexData::ExportMexArray( mri_data );
	mxArray* input_struct = 0;
	mxArray* meas_struct = 0;
	mxArray* result = 0;

	try
	{
		CreateParamsStructure( &input_struct );
		CreateMeasStructure( meas_vector, &meas_struct );

		// put variables
		if( engPutVariable( mat_engine, "k_space", mx_data ) != 0 )
		{
			GIRLogger::LogError( "Plugin_Matlab::Reconstruct -> engPutVariable failed for k_space!\n", script_stream.str().c_str() );
			succeeded = false;
		}
		if( succeeded && engPutVariable( mat_engine, "input_struct", input_struct ) != 0 )
		{
			GIRLogger::LogError( "Plugin_Matlab::Reconstruct -> engPutVariable failed for input_struct!\n", script_stream.str().c_str() );
			succeeded = false;
		}
		if( succeeded && engPutVariable( mat_engine, "meas_struct", meas_struct ) != 0 )
		{
			GIRLogger::LogError( "Plugin_Matlab::Reconstruct -> engPutVariable failed for meas_struct!\n", script_stream.str().c_str() );
			succeeded = false;
		}

		// execute the script
		if( succeeded && engEvalString( mat_engine, script_stream.str().c_str() ) != 0 )
		{
			GIRLogger::LogError( "Plugin_Matlab::Reconstruct -> engEvalString failed!\n", script_stream.str().c_str() );
			succeeded = false;
		}

		// get result
		if( succeeded && ( result = engGetVariable( mat_engine ,"result" ) ) == 0 )
		{
			GIRLogger::LogError( "Plugin_Matlab::Reconstruct -> Matlab variable \"result\" never set, reconstruction failed!\n", script_stream.str().c_str() );
			succeeded = false;
		}
	}
	catch( ... )
	{
		mxDestroyArray( mx_data );
		if( input_struct != 0 )
			mxDestroyArray( input_struct );
		if( meas_struct != 0 )
			mxDestroyArray( meas_struct );
		if( result != 0 )
			mxDestroyArray( result );
		if( mat_engine != 0 )
			engClose( mat_engine );
		GIRLogger::LogError( "Plugin_Matlab::Reconstruct -> Matlab engine threw an exception, reconstruction failed!\n" );
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

bool Plugin_Matlab::Reconstruct( MRIData& mri_data )
{
	// just call other reconstruct method with an empty meas vector
	std::vector<MRIMeasurement> meas_vector;
	return Reconstruct( meas_vector, mri_data );
}

void Plugin_Matlab::CreateParamsStructure( mxArray** input_struct )
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
	*input_struct = mxCreateStructArray( struct_dims, &struct_dims, params.size(), field_names );
	i = 0;
	for( it = params.begin(); it != params.end(); it++ )
	{
		mxSetFieldByNumber( *input_struct, 0, i, mxCreateString( it->second.c_str() ) );
		i++;
	}
}

void Plugin_Matlab::CreateMeasStructure( std::vector<MRIMeasurement>& meas_vector, mxArray** meas_struct )
{
	// create structure
	const char* field_names [] = { 
		"measurement",
		"indicies"
	};
	mwSize struct_dims = meas_vector.size();
	*meas_struct = mxCreateStructArray( 1, &struct_dims, 2, field_names );

	// fill structure
	int num_dims = MRIDimensions::GetNumDims();
	for( unsigned int i = 0; i < meas_vector.size(); i++ )
	{
		// fill measurement mxArray
		mxSetFieldByNumber( *meas_struct, i, 0, MexData::ExportMexArray( meas_vector[i].GetData() ) );

		// create index mxArray
		//MRIDimensions data_size = meas_vector[i].GetData().Size();
		MRIDimensions data_size = meas_vector[i].GetIndex();
		mwSize dims = num_dims + 1; // we need all the dim indicies plus meas time
		mxArray* meas_array = mxCreateNumericArray( 1, &dims, mxSINGLE_CLASS, mxREAL );
		float* index_data = (float*)mxGetPr( meas_array );
		int dim_size;
		for( int j = 0; j < num_dims; j++ )
		{
			data_size.GetDim( j, dim_size );
			// 1 based matlab matrix indicies...
			index_data[j] = dim_size+1;
		}
		index_data[num_dims] = meas_vector[i].meas_time;
		mxSetFieldByNumber( *meas_struct, i, 1, meas_array );
	}

	GIRLogger::LogDebug( "### done loading meas data in struct...\n" );
}
