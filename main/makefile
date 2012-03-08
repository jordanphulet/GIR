CXX_FLAGS := -Wall -Isrc

# fftw_flags
FFTW_PATH := /home/mirl/jhulet/opt/fftw3
FFTW_INC := -I${FFTW_PATH}/include
FFTW_LIB := -L${FFTW_PATH}/lib -lfftw3f
FFTW_ALL := ${FFTW_INC} ${FFTW_LIB}

# matlab flags
MEX_EXT := mexa64
MATLAB_PATH := /opt/LOCAL/matlab/2008b
MEX_BIN := ${MATLAB_PATH}/bin/mex
MATLAB_INC := -I${MATLAB_PATH}/extern/include
MATLAB_LIB := -L${MATLAB_PATH}/bin/glnxa64 -lmex -leng -Wl,-rpath-link,${MATLAB_PATH}/bin/glnxa64 -lmex -leng
MATLAB_ALL := ${MATLAB_INC} ${MATLAB_LIB}

# IDL flags
IDL_PATH := /opt/LOCAL/RSI/8.1/idl
IDL_INC := -I${IDL_PATH}/external/include

# objs
TINYXML_OBJS := src/tinyxml/tinystr.o src/tinyxml/tinyxml.o src/tinyxml/tinyxmlerror.o src/tinyxml/tinyxmlparser.o
BASE_OBJS := src/SiemensTool.o src/GIRUtils.o src/GIRLogger.o src/MRIData.o src/FileCommunicator.o src/TCPCommunicator.o src/DataCommunicator.o src/Serializable.o src/MRIDataComm.o src/RadialGridder.o src/GIRConfig.o src/MRIDataSplitter.o
SERVER_OBJS := src/PMUData.o src/DataSorter.o ${TINYXML_OBJS} src/GIRXML.o src/GIRServer.o src/ReconPipeline.o src/ReconPlugin.o src/MRIDataTool.o src/FilterTool.o src/matlab/MexData.o
ALL_OBJS = ${BASE_OBJS} ${SERVER_OBJS}

all: daemon plugins mex libs

# objects
%.o: %.cpp
	${CXX} ${CXX_FLAGS} ${FFTW_INC} ${MATLAB_INC} ${IDL_INC} -fPIC -c -o $@ $<

# libs
libs: lib/libgir-base.so

lib/libgir-base.so: ${BASE_OBJS}
	${CXX} ${CXX_FLAGS} ${MATLAB_INC} -shared -fPIC -o $@ ${BASE_OBJS}

# daemon
daemon: bin/gir-daemon

bin/gir-daemon: src/gir_daemon.cpp ${ALL_OBJS}
	${CXX} ${CXX_FLAGS} -rdynamic -fPIC -Wl,-E src/gir_daemon.cpp ${ALL_OBJS} ${FFTW_ALL} ${MATLAB_ALL} -o bin/gir-daemon

bin/test-prog: src/test-prog.cpp ${ALL_OBJS}
	${CXX} ${CXX_FLAGS} -fPIC src/test-prog.cpp ${ALL_OBJS} ${FFTW_ALL} ${MATLAB_ALL} -o bin/test-prog

# plugins
plugins: src/plugins/Plugin_Matlab.so src/plugins/Plugin_Nothing.so src/plugins/Plugin_SortCombine.so

%.so: %.cpp
	${CXX} ${CXX_FLAGS} ${MATLAB_INC} -shared -fPIC -o $@ $<
	@cp $@ plugins/

RECV_OBJS := src/GIRUtils.o src/GIRLogger.o src/MRIData.o src/TCPCommunicator.o src/DataCommunicator.o src/Serializable.o src/MRIDataComm.o src/GIRConfig.o
src/matlab/RecvDat.mexa64: src/matlab/RecvDat.cpp src/matlab/MexData.o ${RECV_OBJS}
	${MEX_BIN} -Isrc -Isrc/matlab ${RECV_OBJS} src/matlab/MexData.o -o $@ $<
	@cp $@ bin

# mex
mex: src/matlab/SendDat.${MEX_EXT} src/matlab/RecvDat.${MEX_EXT} src/matlab/LoadDat.${MEX_EXT} src/matlab/GIRTest.${MEX_EXT} src/matlab/SerializeData.${MEX_EXT} src/matlab/UnserializeData.${MEX_EXT}

%.${MEX_EXT}: %.cpp src/matlab/MexData.o ${BASE_OBJS}
	${MEX_BIN} -Isrc -Isrc/matlab ${BASE_OBJS} src/matlab/MexData.o -o $@ $<
	@cp $@ bin

# idl
RECV_OBJS := src/GIRUtils.o src/GIRLogger.o src/MRIData.o src/TCPCommunicator.o src/DataCommunicator.o src/Serializable.o src/MRIDataComm.o src/GIRConfig.o

idl: src/idl/idliceclient.so

src/idl/idliceclient.so: src/idl/RecvDat.cpp src/idl/IDLData.o ${RECV_OBJS}
	${CXX} -shared -fPIC -Isrc -Isrc/idl -I${IDL_PATH}/external/include ${RECV_OBJS} src/idl/IDLData.o -o $@ $<

# clean
clean:
	rm -f bin/{gir-daemon,*.${MEX_EXT}} 
	rm -f src/*.o
	rm -f src/matlab/*.o src/matlab/*.${MEX_EXT}
	rm -f src/idl/*.so
	rm -f src/plugins/*.o src/plugins/*.so
	rm -f src/tinyxml/*.o
