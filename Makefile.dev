BP = bpv7

all:	with$(BP)

withbpv6:
	gmake -C ici all
	gmake -C ici install
	gmake -C ltp all
	gmake -C ltp install
	gmake -C dgr all
	gmake -C dgr install
	gmake -C bssp all
	gmake -C bssp install
	gmake -C $(BP) all
	gmake -C $(BP) install
	gmake -C ams all
	gmake -C ams install
	gmake -C cfdp all
	gmake -C cfdp install
	gmake -C bss all
	gmake -C bss install
	gmake -C dtpc all
	gmake -C dtpc install
	gmake -C nm all
	gmake -C nm install
	gmake -C restart all BP=$(BP)
	gmake -C restart install BP=$(BP)

withbpv7:
	gmake -C ici all
	gmake -C ici install
	gmake -C ltp all
	gmake -C ltp install
	gmake -C dgr all
	gmake -C dgr install
	gmake -C bssp all
	gmake -C bssp install
	gmake -C $(BP) all
	gmake -C $(BP) install
	gmake -C ams all
	gmake -C ams install
	gmake -C cfdp all
	gmake -C cfdp install
	gmake -C bss all
	gmake -C bss install
	gmake -C dtpc all
	gmake -C dtpc install
#	gmake -C nm all
#	gmake -C nm install
	gmake -C restart all BP=$(BP)
	gmake -C restart install BP=$(BP)

clean:
	gmake -C ici clean
	gmake -C ltp clean
	gmake -C dgr clean
	gmake -C bssp clean
	gmake -C $(BP) clean
	gmake -C ams clean
	gmake -C cfdp clean
	gmake -C bss clean
	gmake -C dtpc clean
	gmake -C nm clean
	gmake -C restart clean BP=$(BP)

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
