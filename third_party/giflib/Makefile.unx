#
# Top-level Unix makefile for the GIFLIB package
# Should work for all Unix versions
#
VERSION=3.0

all:
	cd lib; make static shared
	cd util; make all

install:
	cd lib; make install-lib
	cd util; make install-all

uninstall:
	cd lib; make uninstall-lib
	cd util; make uninstall-all

clean:
	cd lib; make clean
	cd util; make clean
	rm -f giflib-$(VERSION)-1.i386.rpm giflib-$(VERSION)-1.src.rpm \
		giflib-$(VERSION).tar.gz core giflib.lsm

giflib-$(VERSION).tar:
	(cd ..; tar cvf giflib-$(VERSION)/giflib-$(VERSION).tar `cat giflib-$(VERSION)/MANIFEST | sed "/\(^\| \)/s// giflib-$(VERSION)\//g"`)
giflib-$(VERSION).tar.gz: giflib-$(VERSION).tar
	gzip -f giflib-$(VERSION).tar

dist: Makefile
	make giflib-$(VERSION).tar.gz
	lsmgen.sh $(VERSION) `wc -c giflib-$(VERSION).tar.gz` >giflib.lsm
	make giflib-$(VERSION).tar.gz
	ls -l giflib-$(VERSION).tar.gz
	@echo "Don't forget to build RPMs from root!"

SHAROPTS = -l63 -n giflib -o giflib -a -s esr@snark.thyrsus.com 
giflib-$(VERSION).shar:
	(cd ..; shar $(SHAROPTS) `cat MANIFEST | sed "/\(^\| \)/s// bs-$(VERSION)\//g"`)

# You need to be root to make this work
RPMROOT=/usr/src/redhat
RPM = rpm
RPMFLAGS = -ba
srcdir = .
rpm: dist
	cp giflib-$(VERSION).tar.gz $(RPMROOT)/SOURCES;
	$(srcdir)/specgen.sh $(VERSION) >$(RPMROOT)/SPECS/giflib.spec
	cd $(RPMROOT)/SPECS; $(RPM) $(RPMFLAGS) giflib.spec	
	cp $(RPMROOT)/RPMS/`arch|sed 's/i[4-9]86/i386/'`/giflib-$(VERSION)*.rpm $(srcdir)
	cp $(RPMROOT)/SRPMS/giflib-$(VERSION)*.src.rpm $(srcdir)
