#include <TCRIterator.h>
#include <GIRLogger.h>
#include <MRIData.h>

void TCRIterator::Load( float alpha, float beta, float beta_squared, float step_size, MRIData& src_meas_data, MRIData& estimate, MRIData& coil_map, MRIData& lambda_map )
{
	if( temp_dim == TEMP_DIM_PHASE )
		temp_dim_size = src_meas_data.Size().Phase;
	else if( temp_dim == TEMP_DIM_REP )
		temp_dim_size = src_meas_data.Size().Repetition;
	else
	{
		GIRLogger::LogError( "TCRIterator::Load -> unsupported temp_dim, defaulting to size of Size().Phase for temp_dim_size.\n" );
		temp_dim_size = src_meas_data.Size().Phase;
	}

	GIRLogger::LogDebug( "### temp_dim_size: %d\n", temp_dim_size );
}

void TCRIterator::Iterate( int iterations )
{
	// perform iterations
	for( int i = 0; i < iterations; i++ )
	{
		GIRLogger::LogInfo( "TCRIterator::Iterate -> iteration: %d...\n", i );

		// fidelity sense term
		ApplySensitivity();
		FFT();
		ApplyFidelityDifference();
		IFFT();
		ApplyInvSensitivity();

		// temporal gradient term
		CalcTemporalGradient();
		
		// update estimate
		UpdateEstimate();
	}
	GIRLogger::LogInfo( "TCRIterator::Iterate -> done iterating.\n" );
}

void TCRIterator::Order( MRIData& mri_data, float* dest )
{
	//TODO: this probably isn't a good way to do this
	if( temp_dim == TEMP_DIM_PHASE )
	{
		for( int average = 0; average < mri_data.Size().Average; average++ )
		for( int partition = 0; partition < mri_data.Size().Partition; partition++ )
		for( int segment = 0; segment < mri_data.Size().Segment; segment++ )
		for( int repetition = 0; repetition < mri_data.Size().Repetition; repetition++ )
		for( int echo = 0; echo < mri_data.Size().Echo; echo++ )
		for( int set = 0; set < mri_data.Size().Set; set++ )
		for( int slice = 0; slice < mri_data.Size().Slice; slice++ )
		for( int channel = 0; channel < mri_data.Size().Channel; channel++ )
		for( int phase = 0; phase < mri_data.Size().Phase; phase++ )
		for( int line = 0; line < mri_data.Size().Line; line++ )
		for( int column = 0; column < mri_data.Size().Column; column++ )
		{
			float* data_index = mri_data.GetDataIndex( column, line, channel, set, phase, slice, echo, repetition, partition, segment, average );
			dest[0] = data_index[0];
			if( mri_data.IsComplex() )
				dest[1] = data_index[1];
			dest += ( mri_data.IsComplex() ) ? 2: 1;
		}
	}
	else if( temp_dim == TEMP_DIM_REP )
	{
		GIRLogger::LogDebug( "### ordering with repetitions...\n" );
		for( int average = 0; average < mri_data.Size().Average; average++ )
		for( int partition = 0; partition < mri_data.Size().Partition; partition++ )
		for( int segment = 0; segment < mri_data.Size().Segment; segment++ )
		for( int phase = 0; phase < mri_data.Size().Phase; phase++ )
		for( int echo = 0; echo < mri_data.Size().Echo; echo++ )
		for( int set = 0; set < mri_data.Size().Set; set++ )
		for( int slice = 0; slice < mri_data.Size().Slice; slice++ )
		for( int channel = 0; channel < mri_data.Size().Channel; channel++ )
		for( int repetition = 0; repetition < mri_data.Size().Repetition; repetition++ )
		for( int line = 0; line < mri_data.Size().Line; line++ )
		for( int column = 0; column < mri_data.Size().Column; column++ )
		{
			float* data_index = mri_data.GetDataIndex( column, line, channel, set, phase, slice, echo, repetition, partition, segment, average );
			dest[0] = data_index[0];
			if( mri_data.IsComplex() )
				dest[1] = data_index[1];
			dest += ( mri_data.IsComplex() ) ? 2: 1;
		}
	}
}

void TCRIterator::Unorder( MRIData& mri_data, float* source )
{
	if( temp_dim == TEMP_DIM_PHASE )
	{
		for( int average = 0; average < mri_data.Size().Average; average++ )
		for( int partition = 0; partition < mri_data.Size().Partition; partition++ )
		for( int segment = 0; segment < mri_data.Size().Segment; segment++ )
		for( int repetition = 0; repetition < mri_data.Size().Repetition; repetition++ )
		for( int echo = 0; echo < mri_data.Size().Echo; echo++ )
		for( int set = 0; set < mri_data.Size().Set; set++ )
		for( int slice = 0; slice < mri_data.Size().Slice; slice++ )
		for( int channel = 0; channel < mri_data.Size().Channel; channel++ )
		for( int phase = 0; phase < mri_data.Size().Phase; phase++ )
		for( int line = 0; line < mri_data.Size().Line; line++ )
		for( int column = 0; column < mri_data.Size().Column; column++ )
		{
			float* data_index = mri_data.GetDataIndex( column, line, channel, set, phase, slice, echo, repetition, partition, segment, average );
			data_index[0] = source[0];
			if( mri_data.IsComplex() )
				data_index[1] = source[1];
			source += ( mri_data.IsComplex() ) ? 2: 1;
		}
	}
	else if( temp_dim == TEMP_DIM_REP )
	{
		GIRLogger::LogDebug( "### unordering with repetitions...\n" );
		for( int average = 0; average < mri_data.Size().Average; average++ )
		for( int partition = 0; partition < mri_data.Size().Partition; partition++ )
		for( int segment = 0; segment < mri_data.Size().Segment; segment++ )
		for( int phase = 0; phase < mri_data.Size().Phase; phase++ )
		for( int echo = 0; echo < mri_data.Size().Echo; echo++ )
		for( int set = 0; set < mri_data.Size().Set; set++ )
		for( int slice = 0; slice < mri_data.Size().Slice; slice++ )
		for( int channel = 0; channel < mri_data.Size().Channel; channel++ )
		for( int repetition = 0; repetition < mri_data.Size().Repetition; repetition++ )
		for( int line = 0; line < mri_data.Size().Line; line++ )
		for( int column = 0; column < mri_data.Size().Column; column++ )
		{
			float* data_index = mri_data.GetDataIndex( column, line, channel, set, phase, slice, echo, repetition, partition, segment, average );
			data_index[0] = source[0];
			if( mri_data.IsComplex() )
				data_index[1] = source[1];
			source += ( mri_data.IsComplex() ) ? 2: 1;
		}
	}
}
