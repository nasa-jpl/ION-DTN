all:
	gmake -C ici $@
	gmake -C ici install
	gmake -C ltp $@
	gmake -C ltp install
	gmake -C dgr $@
	gmake -C dgr install
	gmake -C bssp $@
	gmake -C bssp install
	gmake -C bp $@
	gmake -C bp install
	gmake -C nm $@
	gmake -C nm install
	gmake -C ams $@
	gmake -C ams install
	gmake -C cfdp $@
	gmake -C cfdp install
	gmake -C bss $@
	gmake -C bss install
	gmake -C dtpc $@
	gmake -C dtpc install
	gmake -C restart $@
	gmake -C restart install

clean:
	gmake -C ici $@
	gmake -C ltp $@
	gmake -C dgr $@
	gmake -C bssp $@
	gmake -C bp $@
	gmake -C nm $@
	gmake -C ams $@
	gmake -C cfdp $@
	gmake -C bss $@
	gmake -C dtpc $@
	gmake -C restart $@

test:
	cd tests && ./runtestset normaltests

test-all:
	cd tests && ./runtestset alltests

test-branch:
	@echo
	@echo "You need mercurial (hg) installed for this."
	@echo
	cd tests && hg branch | xargs -L1 ./runtestset

test-%:
	cd tests && ./runtestset $*

retest:
	cd tests && ./runtestset retest


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
