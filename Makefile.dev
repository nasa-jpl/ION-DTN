BP = bpv7

SCRIPTS= ionstart ionstop killm ionscript ionstart.awk 

$(info Development Makefiles actively maintained for only for "bpv7" on platform "i86_64-fedora" options)
$(info  - All other options (bpv6 and platforms) are informational only.)
$(info ION ROOT: ADD_FLAGS is $(ADD_FLAGS))


all:	with$(BP)

withbpv6:
	gmake -C ici all ADD_FLAGS="$(ADD_FLAGS)" 
	gmake -C ltp all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C dgr all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C bssp all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C $(BP) all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C ams all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C cfdp all BP=$(BP) ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C bss all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C dtpc all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C nm all BP=$(BP) ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C restart all BP=$(BP) ADD_FLAGS="$(ADD_FLAGS)"

withbpv7:
	gmake -C ici all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C ltp all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C dgr all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C bssp all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C $(BP) all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C ams all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C cfdp all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C bss all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C dtpc all ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C nm all BP=$(BP) ADD_FLAGS="$(ADD_FLAGS)"
	gmake -C restart all BP=$(BP) ADD_FLAGS="$(ADD_FLAGS)"

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

install:
	gmake -C ici install
	gmake -C ltp install
	gmake -C dgr install
	gmake -C bssp install
	gmake -C $(BP) install
	gmake -C ams install
	gmake -C cfdp install
	gmake -C bss install
	gmake -C dtpc install
	gmake -C nm install BP=$(BP)
	gmake -C restart install BP=$(BP)
	for file in $(SCRIPTS); \
		do cp ./$(notdir $$file) /usr/local/bin; done

uninstall:
	gmake -C ici uninstall
	gmake -C ltp uninstall
	gmake -C dgr uninstall
	gmake -C bssp uninstall
	gmake -C $(BP) uninstall
	gmake -C ams uninstall
	gmake -C cfdp uninstall
	gmake -C bss uninstall
	gmake -C dtpc uninstall
	gmake -C nm uninstall
	gmake -C restart uninstall BP=$(BP)
	for file in $(SCRIPTS); \
		do rm /usr/local/bin/$(notdir $$file); done

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
