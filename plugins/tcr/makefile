PIPELINE := tcr.xml
PIPELINE2 := radial_pc_tcr.xml
MAT_SCRIPT := tcr_coil_combine.m

GIR_INC_DIR := ../../main/src
GIR_LIB_DIR := ../../main/lib
GIR_PLUGINS_DIR := ../../main/plugins

CUDA_LIB_DIR=/usr/local/cuda/lib64
CUDA_INC_DIR=/usr/local/cuda/include

# fftw_flags
FFTW_PATH := /home/mirl/jhulet/opt/fftw3
FFTW_LIB := -L${FFTW_PATH}/lib -lfftw3f

CXX_ARGS := -I./ -I${GIR_INC_DIR}

# CPU / CUDA
Plugin_TCR.so: Plugin_TCR.cpp TCRIterator.o TCRIteratorCPU.o TCRIteratorCUDA.o KernelCode_CPU.o KernelCode_CUDA.o
	${CXX} ${CXX_ARGS} -L${GIR_LIB_DIR} -L${CUDA_LIB_DIR} -I${CUDA_INC_DIR} TCRIterator.o TCRIteratorCPU.o TCRIteratorCUDA.o KernelCode_CUDA.o KernelCode_CPU.o -lgir-base -lcudart -lcufft -shared -fPIC -o $@ $<
	#cp $@ ${GIR_PLUGINS_DIR}

# CPU only
#Plugin_TCR.so: Plugin_TCR.cpp TCRIterator.o TCRIteratorCPU.o KernelCode_CPU.o
#	${CXX} ${CXX_ARGS} -DNO_CUDA -L${GIR_LIB_DIR} -I${CUDA_INC_DIR} TCRIterator.o TCRIteratorCPU.o KernelCode_CPU.o -lgir-base -shared -fPIC -o $@ $<
#	cp $@ ${GIR_PLUGINS_DIR}

%.o: %.cpp
	${CXX} ${CXX_ARGS} -fPIC -c -o $@ $<

KernelCode_CPU.o: KernelCode.cpp
	${CXX} ${CXX_ARGS} -DTCR_KERNEL_CPU -fPIC -c -o $@ $<

TCRIteratorCPU.o: TCRIteratorCPU.cpp
	${CXX} ${CXX_ARGS} -DTCR_KERNEL_CPU -fPIC -c -o $@ $<

KernelCode_CUDA.o: KernelCode.cu
	nvcc ${CXX_ARGS} -DTCR_KERNEL_CUDA -c -o $@ $< --compiler-options -fPIC

TCRIteratorCUDA.o: TCRIteratorCUDA.cu
	nvcc ${CXX_ARGS} -DTCR_KERNEL_CUDA -c -o $@ $< --compiler-options -fPIC

# CPU / CUDA
mpi-tcr: mpi-tcr.cpp TCRIterator.o TCRIteratorCPU.o MPITools.cpp KernelCode_CPU.o KernelCode_CUDA.o MPIPartitioner.o TCRIteratorCUDA.o
	mpicxx TCRIterator.o MPITools.cpp TCRIteratorCPU.o TCRIteratorCUDA.o KernelCode_CPU.o KernelCode_CUDA.o MPIPartitioner.o -limf -lm -I${CUDA_INC_DIR} ${CXX_ARGS} -L${GIR_LIB_DIR} -L${CUDA_LIB_DIR} -L${FFTW_PATH}/lib -lcudart -lcufft -lgir-base -lfftw3f -ldl -o $@ $<

# CPU only
#mpi-tcr: mpi-tcr.cpp TCRIterator.o TCRIteratorCPU.o MPITools.cpp KernelCode_CPU.o MPIPartitioner.o
#	mpicxx -DNO_CUDA TCRIterator.o MPITools.cpp TCRIteratorCPU.o KernelCode_CPU.o MPIPartitioner.o ${CXX_ARGS} -L${GIR_LIB_DIR} -L${FFTW_PATH}/lib -lgir-base -lfftw3f -ldl -o $@ $<

link: remove
	ln -s ${PWD}/${PIPELINE} ${GIR_DIR}/pipelines/${PIPELINE}
	ln -s ${PWD}/${PIPELINE2} ${GIR_DIR}/pipelines/${PIPELINE2}
	ln -s ${PWD}/${MAT_SCRIPT} ${GIR_DIR}/matlab_scripts/${MAT_SCRIPT}
	ln -s ${PWD}/Plugin_TCR.so ${GIR_DIR}/plugins/Plugin_TCR.so

remove:
	rm -f ${GIR_DIR}/pipelines/${PIPELINE}
	rm -f ${GIR_DIR}/pipelines/${PIPELINE2}
	rm -f ${GIR_DIR}/matlab_scripts/${MAT_SCRIPT}
	rm -f ${GIR_DIR}/plugins/Plugin_TCR.so

clean:
	rm -f *.o
	rm -f *.so
	rm -f mpi-tcr
