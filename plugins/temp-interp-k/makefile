GIR_INC_DIR := /v/raid5/usr/jhulet/src/gir/main/src
GIR_LIB_DIR := /v/raid5/usr/jhulet/src/gir/main/lib
GIR_DIR := /v/raid5/usr/jhulet/gir

CXX_ARGS := -I./ -I${GIR_INC_DIR}

Plugin_TempInterpK.so: Plugin_TempInterpK.cpp
	${CXX} ${CXX_ARGS} -L${GIR_LIB_DIR} -lgir-base -shared -fPIC -o $@ $<

link: unlink 
	ln -s ${PWD}/Plugin_TempInterpK.so ${GIR_DIR}/plugins/
	ln -s ${PWD}/temp_interp_k.xml ${GIR_DIR}/pipelines/

unlink:
	rm -f ${GIR_DIR}/plugins/Plugin_TempInterpK.so
	rm -f ${GIR_DIR}/pipelines/temp_interp_k.xml

clean:
	rm -f Plugin_TempInterpK.so
