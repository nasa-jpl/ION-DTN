#University of Bologna
all:
	(mkdir -p build_debug && cd build_debug/ && cmake .. -DDEBUG:STRING=ON && make)
#	(mkdir -p build_debug ; cd build_debug/ ; cmake .. -DDEBUG:STRING=OFF ; make)
#If you compile for debug, the lib will be created in openfec/bin/Debug
#If you compile without debug, the lib will be created in openfec/bin/Release
#If you compile Openfec from  ECLSA, it is assumed that you compile for debug 
#To change this you need to modify both Makefile.in and Makefile.am 
install:
	cp -d bin/Debug/libopenfec.so* /usr/lib/
clean:
	(cd build_debug/ && make clean) || true
	rm -rf build_debug/* || true
uninstall:
	rm -f /usr/lib/libopenfec.so*
distclean: clean
