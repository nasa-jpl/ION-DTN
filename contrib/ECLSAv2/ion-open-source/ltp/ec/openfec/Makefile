all:
	(cd build_debug/; cmake .. -DDEBUG:STRING=ON ; make)
install:
	(cd build_debug/; cmake .. -DDEBUG:STRING=ON ; make)
	cp -d bin/Debug/libopenfec.so* /usr/lib/
clean:
	(cd build_debug/; make clean)
	rm -f /usr/lib/libopenfec.so*
	rm -rf build_debug/*
