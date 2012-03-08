#ifndef TCR_ITERATOR_H
#define TCR_ITERATOR_H

class MRIData;
class MRIDimensions;

class TCRIterator
{
	public:
	enum TemporalDimension { TEMP_DIM_PHASE, TEMP_DIM_REP };

	TCRIterator( TemporalDimension new_temp_dim ): temp_dim( new_temp_dim ) {}
	virtual ~TCRIterator() {}

	virtual void Load( float alpha, float beta, float beta_squared, float step_size, MRIData& src_meas_data, MRIData& estimate, MRIData& coil_map, MRIData& lambda_map );
	virtual void Unload( MRIData& estimate ) = 0;
	virtual void Iterate( int iterations );

	protected:
	TemporalDimension temp_dim;
	int temp_dim_size;

	void Order( MRIData& mri_data, float* dest );
	void Unorder( MRIData& mri_data, float* source );

	virtual void ApplySensitivity() = 0;
	virtual void ApplyInvSensitivity() = 0;
	virtual void FFT() = 0;
	virtual void IFFT() = 0;
	virtual void ApplyFidelityDifference() = 0;
	virtual void CalcTemporalGradient() = 0;
	virtual void UpdateEstimate() = 0;
};

#endif
