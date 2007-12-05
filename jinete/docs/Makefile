default: all

DIR = ../../docs

MAKEDOC = $(DIR)/makedoc

$(DIR)/makedoc-src/makedoc/%.o: $(DIR)/makedoc-src/makedoc/%.c
	gcc -g -o $@ -c $<

$(MAKEDOC): $(DIR)/makedoc-src/makedoc/makechm.o \
	    $(DIR)/makedoc-src/makedoc/makedevh.o \
	    $(DIR)/makedoc-src/makedoc/makedoc.o \
	    $(DIR)/makedoc-src/makedoc/makehtml.o \
	    $(DIR)/makedoc-src/makedoc/makeman.o \
	    $(DIR)/makedoc-src/makedoc/makemisc.o \
	    $(DIR)/makedoc-src/makedoc/makertf.o \
	    $(DIR)/makedoc-src/makedoc/makesci.o \
	    $(DIR)/makedoc-src/makedoc/maketexi.o \
	    $(DIR)/makedoc-src/makedoc/maketxt.o
	gcc -g -o $@ $^

%.txt: %._tx $(MAKEDOC)
	$(MAKEDOC) -ascii $@ $<

%.html: %._tx $(MAKEDOC)
	$(MAKEDOC) -html $@ $<

%.texi: %._tx $(MAKEDOC)
	$(MAKEDOC) -texi $@ $<

%.info: %.texi
	makeinfo --force $<

all: $(MAKEDOC) \
   jinete-es.txt jinete-es.html jinete-es.texi jinete-es.info
