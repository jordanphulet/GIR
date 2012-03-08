#ifndef __MRI_DATA_H__
#define __MRI_DATA_H__

#include <string>

class MRIDimensions
{
	public:
	int Column;
	int Line;
	int Channel;
	int Set;
	int Phase;
	int Slice;

	int Echo;
	int Repetition;
	int Segment;
	int Partition;
	int Average;

	MRIDimensions();
	MRIDimensions( int new_column, int new_line, int new_channel, int new_set, int new_phase, int new_slice, int new_echo, int new_repetition, int new_segment, int new_partition, int new_average );

	int GetProduct() const { return Column * Line * Channel * Set * Phase * Slice * Echo * Repetition * Segment * Partition * Average; }
	bool IndexInBounds( const MRIDimensions& index ) const;
	bool Equals( const MRIDimensions& mri_dimensions ) const;

	static int GetNumDims() { return 11; }

	bool GetDim( int dim_index, int& value );
	bool SetDim( int dim_index, int value );

	std::string ToString() const;
};

class MRIData
{
	public:
	MRIData();
	MRIData( const MRIData& mri_data );
	MRIData( const MRIDimensions& size, bool new_is_complex );

	virtual ~MRIData();

	MRIData& operator = ( const MRIData &data );
	void Copy( const MRIData& mri_data );
	float GetMax();
	void ScaleMax( float new_max );
	bool SetAll( float value );
	bool Add( float value );
	bool Mult( float value );
	void MirrorColumns();

	const MRIDimensions& Size() const { return size; }
	bool IsComplex() const { return is_complex; }
	int NumElements() const { return num_elements; }
	int NumPixels() const { return num_pixels; }

	float* GetDataStart() const { return data; }
	float* GetDataIndex( int column, int line, int channel, int set, int phase, int slice, int echo, int repetition, int partition, int segment, int average ) const;
	float* GetDataIndex( MRIDimensions& index ) const;

	void GetMagnitude( MRIData& mag_data );
	void MakeAbs();

	protected:
	float* data;
	MRIDimensions size;
	bool is_complex;

	// this is a hack for now to get cineTSE working
	// this MRIData is not responsible for initializing or deleting time_data
	MRIData* time_data;

	void Initialize();

	private:
	int slice_size;
	int phase_size;
	int set_size;
	int channel_size;

	int echo_size;
	int repetition_size;
	int partition_size;
	int segment_size;
	int average_size;

	int num_pixels;
	int num_elements;
};

#endif
