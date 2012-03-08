#include <MPITools.h>
#include <MRIData.h>
#include <GIRLogger.h>
#include <mpi.h>

bool MPITools::SendData( MRIData& data, int rep_offset, int rank, MPI_Comm comm )
{
	MRIDimensions dims = data.Size();

	// initialize buffers
	int header_buffer_size = dims.GetNumDims() + 2;
	int header_buffer[header_buffer_size];

	// pack dims
	int dim_size = 0;
	for( int i = 0; i < dims.GetNumDims(); i++ )
	{
		if( !dims.GetDim( i, dim_size ) )
		{
			GIRLogger::LogError( "MPITools::SendData -> couldn't get dim %d!\n", i );
			return false;
		}
		header_buffer[i] = dim_size;
	}

	// pack complexity
	header_buffer[dims.GetNumDims()] = ( data.IsComplex() )? 1: 0;

	// pack repetition offset
	header_buffer[dims.GetNumDims()+1] = rep_offset;

	if
	( 
		MPI_Send( header_buffer, header_buffer_size, MPI_INT, rank, MRI_HEADER_TAG, comm ) != MPI_SUCCESS || 
		MPI_Send( data.GetDataStart(), data.NumElements(), MPI_FLOAT, rank, MRI_DATA_TAG, comm ) != MPI_SUCCESS 
	)
	{
		GIRLogger::LogError( "MPITools::SendData -> MPI_Send failed!\n" );
		return false;
	}
	else
		return true;
}

bool MPITools::ReceiveData( MRIData& data, int& rep_offset, int rank, MPI_Comm comm )
{
	// initialize header buffers
	MRIDimensions dims;
	int header_buffer_size = dims.GetNumDims() + 2;
	int header_buffer[header_buffer_size];

	// get header
	MPI_Status status;
	if( MPI_Recv( header_buffer, header_buffer_size, MPI_INT, rank, MRI_HEADER_TAG, comm, &status ) != MPI_SUCCESS )
	{
		GIRLogger::LogError( "MPITools::ReceiveData -> MPI_Recv failed for header, from rank %d...\n", rank );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
		return false;
	}

	// unpack dims
	for( int i = 0; i < dims.GetNumDims(); i++ )
		if( !dims.SetDim( i, header_buffer[i] ) )
			return false;

	//GIRLogger::LogDebug( "### recvd header\n" );
	//GIRLogger::LogDebug( "### header dims: %s\n", dims.ToString().c_str() ); 

			
	// unpack complexity
	bool is_complex = header_buffer[dims.GetNumDims()] == 1;

	// pack repetition offset
	rep_offset = header_buffer[dims.GetNumDims()+1];


	// initialize data
	data = MRIData( dims, is_complex );

	// receive data
	//int data_buffer_size = ( data.IsComplex() )? dims.GetProduct() * 2: dims.GetProduct();
	int status2 = MPI_Recv( data.GetDataStart(), data.NumElements(), MPI_FLOAT, rank, MRI_DATA_TAG, comm, &status );
	return status2 == MPI_SUCCESS;
}

/*
bool MPITools::SendParameters( float alpha, float beta, float step_size, int iterations, bool is_pc, int rank, MPI_Comm comm )
{

	// initialize buffer
	int buffer_size = 5;
	float buffer[buffer_size];

	// pack
	buffer[0] = alpha;
	buffer[1] = beta;
	buffer[2] = step_size;
	buffer[3] = (float)iterations;
	buffer[5] = ( is_pc )? 1: 0;

	if( MPI_Send( buffer, buffer_size, MPI_FLOAT, rank, MRI_PARAMETERS_TAG, comm ) != MPI_SUCCESS )
	{
		GIRLogger::LogError( "MPITools::SendParameters -> MPI_Send failed!\n" );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
		return false;
	}
	else
		return true;
}

bool MPITools::ReceiveParameters( float& alpha, float& beta, float& step_size, int& iterations, bool& is_pc, int rank, MPI_Comm comm )
{
	// initialize buffer
	int buffer_size = 5;
	float buffer[buffer_size];

	// get parameters
	MPI_Status status;
	if( MPI_Recv( buffer, buffer_size, MPI_FLOAT, rank, MRI_PARAMETERS_TAG, comm, &status ) != MPI_SUCCESS )
	{
		GIRLogger::LogError( "MPITools::ReceiveParameters -> MPI_Recv failed for header, from rank %d...\n", rank );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
		return false;
	}

	// unpack 
	alpha = buffer[0];
	beta = buffer[1];
	step_size = buffer[2];
	iterations = (int)buffer[3];
	is_pc = buffer[4] > 0.9;

	return true;
}
*/
