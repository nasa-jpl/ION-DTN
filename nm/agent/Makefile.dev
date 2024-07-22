SOURCES = ../shared/adm/*.c ../shared/msg/*.c ../shared/utils/*.c ../shared/primitives/*.c ./*.c 
IONDIR = ../../
OPTS = -Wno-write-strings -g 

all:
	gcc ${OPTS} \
	-I. -I.. \
	-I${IONDIR}/ici/include \
	-I${IONDIR}/ltp/include \
	-I${IONDIR}/ltp/library \
	-I${IONDIR}/bp/include \
	-I${IONDIR}/bp/library \
	-I${IONDIR}/ici/library \
	-I/usr/local/include \
	-L${IONDIR}/ \
	-L/usr/local/lib \
	-lm -lbp -lici -lltp \
	-DMAXPATHLEN=2048 \
	-DAGENT_ROLE=1 \
	-c ${SOURCES} 

	gcc -o nm_agent *.o -lm -lltp -lbp -lici -lpthread

