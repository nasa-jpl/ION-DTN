SOURCES = ../shared/adm/*.c ../shared/msg/*.c ../shared/utils/*.c ../shared/primitives/*.c ./*.c 
IONDIR = ../../
OPTS = -Wno-write-strings -g 

all:with-db

without-db:
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
	-lbp -lici -lltp \
	-DMAXPATHLEN=2048 \
	-c ${SOURCES} 

	gcc -o nm_mgr *.o -lltp -lbp -lici -lpthread

with-db:
	gcc ${OPTS} \
	-I. -I.. \
	-I${IONDIR}/ici/include \
	-I${IONDIR}/ltp/include \
	-I${IONDIR}/ltp/library \
	-I${IONDIR}/bp/include \
	-I${IONDIR}/bp/library \
	-I${IONDIR}/ici/library \
	-I/usr/local/include \
	-I/usr/include/mysql \
	-L${IONDIR}/ \
	-L/usr/local/lib \
	-L/usr/lib \
	-lbp -lici -lltp -lmysqlclient \
	-DMAXPATHLEN=2048 \
	-DHAVE_MYSQL=1 \
	-c ${SOURCES} 

	gcc -o nm_mgr *.o -lltp -lbp -lici -lmysqlclient -lpthread



