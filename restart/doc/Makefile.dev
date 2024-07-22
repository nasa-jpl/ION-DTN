PODM1 = pod2man -s 1 -c "Restart executables"
PODH = pod2html --noindex

MANFILES = \
	./man/man1/ionrestart.1

HTMLFILES = \
	./html/man1/ionrestart.html

ALL = $(MANFILES) $(HTMLFILES)

all:		$(ALL)

clean:
		rm -f man/man1/*.1
		rm -f html/man1/*.html
		rm -f *.tmp

install:
		cp man/man1/*.1 $(ROOT)/man/man1

#	-	-	Man files	-	-	-	-	-

./man/man1/%.1:	pod1/%.pod
		$(PODM1) $< $@

./html/man1/%.html:	pod1/%.pod
		$(PODH) --infile=$< --outfile=$@
