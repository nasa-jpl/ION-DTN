SOURCES = ../shared/adm/*.c ../shared/msg/*.c ../shared/utils/*.c ../shared/primitives/*.c ./*.c 
IONDIR = ../../

all:
	g++ -Wno-write-strings -arch i386 -g \
	-I. -I.. \
	-I${IONDIR}/ici/include \
	-I${IONDIR}/ltp/include \
	-I${IONDIR}/ltp/library \
	-I${IONDIR}/bp/include \
	-I${IONDIR}/bp/library \
	-I${IONDIR}/ici/library \
	-I/usr/local/include \
	-L/usr/local/lib \
	-lbp -lici -lltp \
	-DMAXPATHLEN=2048 \
	${SOURCES} \
	-o nm_agent
