#ifndef MPI_TOOLS_H
#define MPI_TOOLS_H

#include <mpi.h>

class MRIData;

const int MRI_HEADER_TAG = 0;
const int MRI_DATA_TAG = 1;
const int MRI_PARAMETERS_TAG = 1;

class MPITools
{
	public:
	static bool SendData( MRIData& data, int rep_offset, int rank, MPI_Comm comm );
	static bool ReceiveData( MRIData& data, int& rep_offset, int rank, MPI_Comm comm );

	/*
	static bool SendParameters( float alpha, float beta, float step_size, int iterations, bool is_pc, int rank, MPI_Comm comm );
	static bool ReceiveParameters( float& alpha, float& beta, float& step_size, int& iterations, bool& is_pc, int rank, MPI_Comm comm );
	*/
};

#endif
