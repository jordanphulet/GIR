PIPELINE := noah.xml
MAT_SCRIPT := noah.m
GIR_DIR := /v/raid5/usr/jhulet/gir

link: remove
	ln -s ${PWD}/${PIPELINE} ${GIR_DIR}/pipelines/${PIPELINE}
	ln -s ${PWD}/${MAT_SCRIPT} ${GIR_DIR}/matlab_scripts/${MAT_SCRIPT}

remove:
	rm -f ${GIR_DIR}/pipelines/${PIPELINE}
	rm -f ${GIR_DIR}/matlab_scripts/${MAT_SCRIPT}
