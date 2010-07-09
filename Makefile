all:
	gmake -C ici $@
	gmake -C ici install
	gmake -C ltp $@
	gmake -C ltp install
	gmake -C dgr $@
	gmake -C dgr install
	gmake -C bp $@
	gmake -C bp install
	gmake -C ams $@
	gmake -C ams install
	gmake -C cfdp $@
	gmake -C cfdp install

clean:
	gmake -C ici $@
	gmake -C ltp $@
	gmake -C dgr $@
	gmake -C bp $@
	gmake -C ams $@
	gmake -C cfdp $@

test:
	cd tests
	./runtests


vxworks5:
	ldppc -r -o ionModule.o				\
	./ici/arch-vxworks5/icilib.o			\
	./ltp/arch-vxworks5/ltplib.o			\
	./ams/arch-vxworks5/amslib.o			\
	./bp/arch-vxworks5/bplib.o			\
	./dgr/arch-vxworks5/dgrlib.o			\
	./vxworks-utils/arch-vxworks5/vxwutils.o	\
	./vxwork-utils/vxwork-expat/libexpat.obj

	chmod 755 ionModule.o
