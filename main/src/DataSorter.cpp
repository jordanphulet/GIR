#include <DataSorter.h>
//#include <Serializable.h>
#include <MRIDataComm.h>
#include <MRIData.h>
#include <GIRLogger.h>
#include <GIRUtils.h>
#include <PMUData.h>
#include <vector>

DataSorter::DataSorter(): sort_method( "reorder" )
{
}

bool DataSorter::Configure( GIRConfig& config, bool main_config, bool final_config )
{
	config.GetParam( 0, 0, "sort_method", sort_method );
	config.GetParam( 0, 0, "sort_script", sort_script );
	if( config.GetParam( 0, 0, "pmu_dir", pmu_dir ) )
	{
		GIRUtils::CompleteDirPath( pmu_dir );
		if( !GIRUtils::IsDir( pmu_dir.c_str() ) )
		{
			GIRLogger::LogError( "DataSorter::Configure-> pmu_dir \"%s\" is invalid!\n", pmu_dir.c_str() );
			return false;
		}
	}
	if( main_config && config.GetParam( 0, 0, "sort_script_dir", sort_script_dir ) )
	{
		GIRUtils::CompleteDirPath( sort_script_dir );
		if( !GIRUtils::IsDir( sort_script_dir.c_str() ) )
		{
			GIRLogger::LogError( "DataSorter::Configure-> sort_script_dir \"%s\" is invalid!\n", sort_script_dir.c_str() );
			return false;
		}
	}

	// TODO: check sort method if final_config


	return true;
}

bool DataSorter::Sort( std::vector<MRIMeasurement>& meas_vector, MRIData& data )
{
	if( sort_method.compare( "reorder" ) == 0 )
		return SortReplace( meas_vector, data );
	else
	{
		GIRLogger::LogError( "DataSorter::Sort -> invalid sort_method: %s!\n", sort_method.c_str() );
		return false;
	}
}

bool DataSorter::SortReplace( std::vector<MRIMeasurement>& meas_vector, MRIData& data )
{
	std::vector<MRIMeasurement>::iterator it;
	for( it = meas_vector.begin(); it != meas_vector.end(); it++ )
		if( !it->UnloadData( data ) )
			GIRLogger::LogError( "DataSorter::SortReplace -> MRIMasurement::UnloadData failed, measurement was not added!\n" );
	return true;
}

/*
bool DataSorter::SortCineTSE( std::vector<MRIMeasurement>& meas_vector, MRIData& data, std::string& pmu_path, int bins )
{
	if( !data.IsComplex() )
	{
		GIRLogger::LogError( "DataSorter::SortCineTSE -> expected complex data, aborting!\n" );
		return false;
	}

	GIRLogger::LogDebug( "### sorting cineTSE data, pmu_path: %s, bins: %d\n", pmu_path.c_str(), bins );
	PMUData pmu_data;
	if( !pmu_data.LoadFile( pmu_path, bins ) )
	{
		GIRLogger::LogError( "DataSorter::SortCineTSE -> unable to load pmu file: %s!\n", pmu_path.c_str() );
		return false;
	}
	GIRLogger::LogDebug( "PMU data:\n%s\n", pmu_data.ToString().c_str() );

	// create new, correctly sized, data
	MRIDimensions new_dims = data.Size();
	new_dims.Phase = bins;
	MRIData new_data( new_dims, true );
	new_data.SetAll( 0 );

	// keep track of weights
	MRIDimensions weight_dims = new_dims;
	weight_dims.Column = 1;
	MRIData weights( weight_dims, false );
	weights.SetAll( 1 );

	std::vector<MRIMeasurement>::iterator it;
	for( it = meas_vector.begin(); it != meas_vector.end(); it++ )
	{
		int bin = pmu_data.GetBin( (int)(it->meas_time * 2.5) );
		MRIDimensions& index = it->GetIndex();
		index.Phase = bin;
		if( !it->UnloadData( new_data ) )
		{
			GIRLogger::LogError( "DataSorter::SortCineTSE -> couldn't insert measurement into data!\n" );
			return false;
		}

		// record weight
		*weights.GetDataIndex( index ) += 1;
	}

	// apply weights
	for( int line = 0; line < new_dims.Line; line++ )
	for( int channel = 0; channel < new_dims.Channel; channel++ )
	for( int set = 0; set < new_dims.Set; set++ )
	for( int phase = 0; phase < new_dims.Phase; phase++ )
	for( int slice = 0; slice < new_dims.Slice; slice++ )
	for( int echo = 0; echo < new_dims.Echo; echo++ )
	for( int repetition = 0; repetition < new_dims.Repetition; repetition++ )
	for( int partition = 0; partition < new_dims.Partition; partition++ )
	for( int segment = 0; segment < new_dims.Segment; segment++ )
	for( int average = 0; average < new_dims.Average; average++ )
	{
		float* data_index = new_data.GetDataIndex( 0, line, channel, set, phase, slice, echo, repetition, partition, segment, average );
	}

	// send on new data
	data = new_data;
	return true;
}
*/
