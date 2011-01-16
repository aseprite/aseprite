#
# This is the make file for the lib subdirectory of the GIF library
# You'll need GNU gcc or some other ANSI-complient compiler.
#
#				Gershon Elber, Jun 1989
#  * version number in ./lib/Makefile
#
V = 3.0

SHLIB = libgif.so.$(V)

#
# Where all the include files are:
INCS = -I.

LIBDIR = /usr/lib

#
# What devices we should be able to grab into gif images. Note that if
# you enable them the associated library must be available at link time.
# Avaiable devices:
# 1. EGA, VGA, SVGA (800x600), Hercules - all on IBM PC only.
# 2. SGI 4D Irix using gl library (Add -D__SGI_GL__).
# 3. X11 using libX.a (Add -D__X11__)
DEVS = -D__X11__

#
# These are the flags for gcc, in BSD4.3 or Sun O.S. 4.0.3
#
# If your system has all function prototypes for gcc, replace all
# the -Wxxx with -Wall. I can not add -Wimplicit as my system uses old cc
# h files.
#
# CC = gcc
#
# CFLAGS = -O -c -W -Wreturn-type -Wcomment
# CFLAGS = -g -pg -c -W -Wreturn-type -Wcomment
#
# for sun 4 (gunnars@ifi.uib.no). Tested using gcc 1.39.
#
# CFLAGS = -O -c -sun4 -W -Wreturn-type -Wcomment -DUSE_VARARGS
# CFLAGS = -g -c -sun4 -W -Wreturn-type -Wcomment -DUSE_VARARGS

#
# These are the flags for cc on SGI iris4d. O.S. IRIX 3.2
#
# CC = cc
#
# CFLAGS = -O -c -DSYSV -DNO_VOID_PTR -DUSE_VARARGS -Olimit 1000 -Wf,-XNh5000 -Wf,-XNd5000 -G 4
# CFLAGS = -g -p -c -DSYSV -DNO_VOID_PTR -DUSE_VARARGS -Olimit 1000 -Wf,-XNh5000 -Wf,-XNd5000 -G 4

#
# These are the flags for xlc, ansi compiler for IBM R6000
#
# CC = xlc
#
# CFLAGS = -O -c -qnoro -D_POSIX_SOURCE -D_ALL_SOURCE -DR6000
# CFLAGS = -g -pg -c -qnoro -D_POSIX_SOURCE -D_ALL_SOURCE -DR6000

#
# GNU CC over Linux and XFree86
#
CC = gcc
#
#CFLAGS = -fno-builtin -O -c -W -Wreturn-type -Wcomment
CFLAGS = -fno-builtin -g -c -W -Wreturn-type -Wcomment


OBJS = dev2gif.o egif_lib.o dgif_lib.o gifalloc.o gif_font.o gif_hash.o \
	gif_err.o quantize.o qprintf.o getarg.o

.c.o:
	$(CC) $(INCS) $(DEVS) $(CFLAGS) $<

all: static shared

static: libgif.a
shared: $(SHLIB)

libgif.a: $(OBJS)
	rm -f libgif.a
	ar rcv libgif.a $(OBJS)
	-ranlib libgif.a

$(SHLIB): $(OBJS)
	rm -f libgif.so.$(V)
	gcc -shared -o $(SHLIB) $(OBJS)

dev2gif.o: gif_lib.h
egif_lib.o: gif_lib.h gif_hash.h
dgif_lib.o: gif_lib.h gif_hash.h
gif_hash.o: gif_lib.h gif_hash.h
gif_err.o: gif_lib.h
gifalloc.o: gif_lib.h
gif_font.o: gif_lib.h
quantize.o: gif_lib.h
qprintf.o: gif_lib.h
getarg.o: getarg.h

install-lib: libgif.a
	cp libgif.a libgif.so.$(V) $(LIBDIR)

uninstall-lib: shared static
	rm -f $(LIBDIR)/libgif.a $(LIBDIR)/$(SHLIB)
clean:
	rm -f *.[oa] *~ libgif.so.$(V)
