#include <FileCommunicator.h>
#include <GIRConfig.h>
#include <GIRLogger.h>
#include <MRIDataTool.h>
#include <Serializable.h>
#include <MPITools.h>
#include <RadialGridder.h>
#include <MRIDataSplitter.h>
#include <MPIPartitioner.h>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <math.h>
#include <mpi.h>

#include <TCRIteratorCPU.h>
#ifndef NO_CUDA
	#include <TCRIteratorCUDA.h>
#endif

bool Reconstruct( MRIData& data, float alpha, float beta, float step_size, int iterations, bool use_gpu, int rep_offset )
{
	GIRLogger::LogInfo( "reconstructing (%s)...\n", data.Size().ToString().c_str() );
	int gpu_thread_load = 2;
	int threads = 16;
	
	//GIRLogger::LogDebug( "### just gridding...\n" );

	// regrid
	{
		RadialGridder gridder;
		gridder.repetition_offset = rep_offset;
		gridder.view_ordering = RadialGridder::VO_GOLDEN_RATIO;
		MRIData temp_data;
		if( !gridder.Grid( data, temp_data, RadialGridder::KERN_TYPE_BILINEAR, 101, true ) )
		{
			GIRLogger::LogError( "Regridding failed, aborting!\n" );
			exit( EXIT_FAILURE );
		}
		data = temp_data;
	}
	
	// shift to corner
	FilterTool::FFTShift( data );

	// generate coil map
	MRIData coil_map;
	GIRLogger::LogInfo( "Plugin_TCR::Reconstruct -> generating coil map...\n" );
	MRIDataTool::GetCoilMap( data, coil_map );

	// generate original estimate
	GIRLogger::LogInfo( "Plugin_TCR::Reconstruct -> generating original estimate...\n" );
	MRIData estimate;
	MRIDataTool::TemporallyInterpolateKSpace( data, estimate );
	FilterTool::FFT2D( estimate, true );
	estimate.MakeAbs();

	// iterate
	if( use_gpu )
	{
#ifndef NO_CUDA
		int rank;
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		GIRLogger::LogDebug( "### cuda device: %d\n", rank % 2 );

		GIRLogger::LogInfo( "Plugin_TCR::Reconstruct -> reconstructing on GPU...\n" );
		TCRIteratorCUDA iterator( gpu_thread_load, TCRIterator::TEMP_DIM_REP );
		iterator.cuda_device = rank % 2;
		iterator.Load( alpha, beta, step_size, data, estimate, coil_map );
		iterator.Iterate( iterations );
		iterator.Unload( data );
#else
		GIRLogger::LogError( "Plugin_TCR::Reconstruct -> GPU requested but binaries not build with CUDA, aborting!\n" );
		return false;
#endif
	}
	else
	{
		GIRLogger::LogInfo( "Plugin_TCR::Reconstruct -> reconstructing on CPU(s)...\n" );
		TCRIteratorCPU iterator( threads, TCRIterator::TEMP_DIM_REP );
		iterator.Load( alpha, beta, step_size, data, estimate, coil_map );
		iterator.Iterate( iterations );
		iterator.Unload( data );
	}

	// shift to center
	FilterTool::FFTShift( data, true );
	return true;
}


void ExecuteMaster( int tasks, const char* input_file, float alpha, float beta, float step_size, int iterations, bool use_gpu )
{
	printf( "parameters:\n\talpha %f, beta %f, step_size %f, iterations %d, use_gpu %d\n", alpha, beta, step_size, iterations, use_gpu );
		
	// open file communicator
	printf( "opening %s...\n", input_file );
	FileCommunicator communicator;
	if( !communicator.OpenInput( input_file ) || !communicator.OpenOutput( "tcr_data.out" ) )
	{
		GIRLogger::LogError( "Unable to open IO files!\n" );
		exit( EXIT_FAILURE );
	}

	// get request, we don't use right now but we need to get it out of the way
	GIRLogger::LogInfo( "Loading data...\n" );
	MRIReconRequest request;
	communicator.ReceiveReconRequest( request );

	// get data
	MRIData data;
	communicator.ReceiveData( data );

	// send data
	int overlap = 3;

	for( int i = tasks-1; i >=0; i-- )
	{
		// split off data
		MRIData split_data;
		int split_start;
		int split_end;
		if( !MPIPartitioner::SplitRepetitions( data, split_data, i, tasks, overlap, split_start, split_end ) )
		{
			GIRLogger::LogError( "SplitRepetitions failed for task: %d!\n", i );
			MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
		}

		// data for master
		if( i == 0 )
		{
			// reconstruct
			Reconstruct( split_data, alpha, beta, step_size, iterations, use_gpu==1, split_start );
			// resize due to gridding
			MRIDimensions new_dims = data.Size();
			new_dims.Line = split_data.Size().Line;
			new_dims.Column = split_data.Size().Column;
			data = MRIData( new_dims, true );
			data.SetAll( 0 );
			// merge
			if( !MPIPartitioner::MergeRepetitions( data, split_data, i, tasks, overlap ) )
			{
				GIRLogger::LogError( "MergeRepetitions failed for task: %d!\n", i );
				MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
			}

		}
		// data for another task
		else
		{
			GIRLogger::LogInfo( "Sending data to %d...\n", i );
			if( !MPITools::SendData( split_data, split_start, i, MPI_COMM_WORLD ) )
			{
				GIRLogger::LogDebug( "SendData to %d failed!\n", i );
				MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
			}
			GIRLogger::LogInfo( "Finished sending data to %d.\n", i );
		}
	}

	// get data from other tasks
	for( int i = 1; i < tasks; i++ )
	{
		GIRLogger::LogInfo( "Receiving data from %d...\n", i );

		// receive
		MRIData split_data;
		int rep_offset; // meaningless in this context...
		if( !MPITools::ReceiveData( split_data, rep_offset, i, MPI_COMM_WORLD ) )
		{
			GIRLogger::LogError( "ReceiveData from task %d failed!\n", i );
			MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
		}

		// merge
		if( !MPIPartitioner::MergeRepetitions( data, split_data, i, tasks, overlap ) )
		{
			GIRLogger::LogError( "MergeRepetitions failed for task: %d!\n", i );
			MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
		}
	}

	// write output
	GIRLogger::LogInfo( "Writing output...\n" );
	communicator.SendData( data );
	GIRLogger::LogInfo( "Done.\n" );
}

void ExecuteSlave( float alpha, float beta, float step_size, int iterations, bool use_gpu )
{
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// recieve data
	MRIData split_data;
	int rep_offset;
	MPITools::ReceiveData( split_data, rep_offset, 0, MPI_COMM_WORLD );

	// reconstruct 
	GIRLogger::LogInfo( "\t(%d) reconstructing...\n", rank );
	Reconstruct( split_data, alpha, beta, step_size, iterations, use_gpu==1, rep_offset );
	GIRLogger::LogInfo( "\t(%d) done reconstructing...\n", rank );

	// send back
	MPITools::SendData( split_data, -1, 0, MPI_COMM_WORLD );
}

int main( int argc, char** argv )
{
	if( argc != 7 )
	{
		fprintf( stderr, "USAGE: mpi-tcr INPUT_FILE ALPHA BETA STEP_SIZE ITERATIONS USE_GPU\n" );
		exit( EXIT_FAILURE );
	}

	// read in arguments
	const char* input_file = argv[1];
	double alpha;
	double beta;
	double step_size;
	int iterations;
	int use_gpu;
	std::stringstream str;
	str << argv[2]  << " " << argv[3] << " " << argv[4] << " " << argv[5] << " " << argv[6];
	str >> alpha >> beta >> step_size >> iterations >> use_gpu;
	
	// initialize MPI
	int rank;
	int tasks;
	MPI_Init( &argc, &argv );
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &tasks);

	// execute
	if( rank == 0 )
	{
		GIRLogger::LogInfo( "task 0 starting, %d total tasks...\n", tasks );
		ExecuteMaster( tasks, argv[1], alpha, beta, step_size, iterations, use_gpu );
	}
	else
	{
		GIRLogger::LogInfo( "task %d starting...\n", rank);
		ExecuteSlave( alpha, beta, step_size, iterations, use_gpu );
	}

	// finalize MPI
	MPI_Finalize();
	exit( EXIT_SUCCESS );
}
