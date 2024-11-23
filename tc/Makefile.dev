OPT = /usr/local
PLATFORMS = i86_64-fedora
# i86-redhat

all:
		cd doc; \
		mkdir -p man; \
		mkdir -p man/man1; \
		mkdir -p man/man3; \
		mkdir -p man/man5; \
		mkdir -p html; \
		mkdir -p html/man1; \
		mkdir -p html/man3; \
		mkdir -p html/man5; \
		gmake all ROOT=$(OPT); \
		cd ..; \
		for PF in $(PLATFORMS); \
			do \
				cd $$PF; \
				mkdir -p bin; \
				mkdir -p lib; \
				gmake all ROOT=$(OPT); \
				cd ..; \
			done

clean:
		cd doc; \
		gmake -i clean; \
		cd ..; \
		for PF in $(PLATFORMS); \
			do cd $$PF; gmake -i clean; cd ..; done

install:
		cd doc; \
		gmake -i install ROOT=$(OPT); \
		cd ..; \
		for PF in $(PLATFORMS); \
			do cd $$PF; gmake -i install ROOT=$(OPT); cd ..; done
