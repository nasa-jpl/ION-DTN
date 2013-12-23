OPT = /opt
PLATFORMS = i86-redhat
# i86_64-fedora
# sparc-sol9
# i86-freebsd

all:
		cd doc; \
		mkdir -p man; \
		mkdir -p man/man1; \
		mkdir -p html; \
		mkdir -p html/man1; \
		gmake all ROOT=$(OPT); \
		cd ..; \
		for PF in $(PLATFORMS);\
			do \
				cd $$PF; \
				mkdir -p bin; \
				gmake all ROOT=$(OPT); \
				cd ..; \
			done

clean:
		cd doc; \
		gmake -i clean; \
		cd ..; \
		for PF in $(PLATFORMS);\
			do cd $$PF; gmake -i clean; cd ..; done

install:
		cd doc; \
		gmake -i install ROOT=$(OPT); \
		cd ..; \
		for PF in $(PLATFORMS);\
			do cd $$PF; gmake -i install ROOT=$(OPT); cd ..; done
