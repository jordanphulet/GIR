GIR_INC_DIR := /v/raid5/usr/jhulet/src/gir/main/src
GIR_LIB_DIR := /v/raid5/usr/jhulet/src/gir/main/lib
PIPELINE := radial_gridder.xml
PIPELINE2 := radial_gridder_dc.xml
MAT_SCRIPT := radial_dc.m
GIR_DIR := /v/raid5/usr/jhulet/gir

CXX_ARGS := -I./ -I${GIR_INC_DIR}

Plugin_RadialGridder.so: Plugin_RadialGridder.cpp
	${CXX} ${CXX_ARGS} -L${GIR_LIB_DIR} -lgir-base -shared -fPIC -o $@ $<

link: unlink 
	ln -s ${PWD}/${PIPELINE} ${GIR_DIR}/pipelines/${PIPELINE}
	ln -s ${PWD}/Plugin_RadialGridder.so ${GIR_DIR}/plugins/
	ln -s ${PWD}/${PIPELINE2} ${GIR_DIR}/pipelines/${PIPELINE2}
	ln -s ${PWD}/${MAT_SCRIPT} ${GIR_DIR}/matlab_scripts/${MAT_SCRIPT}

unlink:
	rm -f ${GIR_DIR}/pipelines/${PIPELINE}
	rm -f ${GIR_DIR}/plugins/Plugin_RadialGridder.so
	rm -f ${GIR_DIR}/pipelines/${PIPELINE2}
	rm -f ${GIR_DIR}/matlab_scripts/${MAT_SCRIPT}

clean:
	rm -f Plugin_RadialGridder.so
