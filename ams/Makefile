OPT = /opt
PLATFORMS = i86-redhat
# i86_64-fedora
# sparc-sol9
# i86-freebsd

all:
		for PF in $(PLATFORMS); \
			do \
				cd $$PF; \
				mkdir -p bin; \
				mkdir -p lib; \
				gmake all ROOT=$(OPT); \
				cd ..; \
			done

clean:
		for PF in $(PLATFORMS); \
			do cd $$PF; gmake -i clean; cd ..; done

install:
		for PF in $(PLATFORMS); \
			do cd $$PF; gmake -i install ROOT=$(OPT); cd ..; done
