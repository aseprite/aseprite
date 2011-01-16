#
# This is the make file for the util subdirectory of the GIF library
# The compiler must be GNU gcc or ANSI-compliant.
#
# To make test versions in the utils directory:
#
#	make -f makefile.unx [all] [rle] [iris]
#
# To install them in the /usr/bin directory:
#
#	make -f makefile.unx [install-all] [install-rle] [install-iris]
#
#				Eric Raymond, September 1992
#

#
# If you have the Utah raster tool kit and wants conversion routines to and
# from it set the ones below properly.
#
# RLE_INC = -I/u/urt/urt3.0/include
# RLE_LIB = /u/urt/urt3.0/lib/librle.a
# X_INC_DIR = /server/sun4/usr/new/lib/X11R4

# Declare the include files and libraries for the GIF utils:
INC = -I../lib
GIF_LIB = ../lib/libgif.a
GIF_INC_DEPEND = ../lib/libgif.a ../lib/gif_lib.h ../lib/getarg.h

# Where to copy executables to:
DEST = /usr/bin

# These are the flags for gcc, in BSD4.3 or Sun O.S. 4.0.3
#
# If your system has all function prototypes for gcc, replace all
# the -Wxxx with -Wall. I can not add -Wimplicit as my system uses old cc
# h files.
#
# CC = gcc
# CFLAGS = $(INC) -O -s -W -Wreturn-type -Wcomment
# CFLAGS = $(INC) -g -pg -W -Wreturn-type -Wcomment


# For sun 4 (gunnars@ifi.uib.no). Tested using gcc 1.39.
#
# CFLAGS = $(INC) -O -s -sun4 -W -Wreturn-type -Wcomment -DUSE_VARARGS
# CFLAGS = $(INC) -g -sun4 -W -Wreturn-type -Wcomment -DUSE_VARARGS
# XLIBS = -lX11

# These are the flags for cc on SGI iris4d. O.S. IRIX 3.2. Note you must
# define XLIBS as well.
#
# CC = cc
# CFLAGS = $(INC) -O -s -DSYSV -DNO_VOID_PTR -Olimit 1000 -Wf,-XNh5000 -Wf,-XNd5000 -G 4
# CFLAGS = $(INC) -g -p -DSYSV -DNO_VOID_PTR -Olimit 1000 -Wf,-XNh5000 -Wf,-XNd5000 -G 4
# XLIBS = -lbsd -lX11

# These are the flags for xlc, ansi compiler for IBM R6000
#
# CC = xlc
# CFLAGS = $(INC) -O -s -qnoro -D_POSIX_SOURCE -D_ALL_SOURCE -DR6000
# CFLAGS = $(INC) -g -pg -qnoro -D_POSIX_SOURCE -D_ALL_SOURCE -DR6000
# XLIBS = -lX11

# GNU CC 2.2 under System V Release 3.2 with AT&T's crocked-up X11R3
#
# CC = gcc
# CFLAGS = $(INC) -fno-builtin -O -s -W -Wreturn-type -Wcomment
# CFLAGS = $(INC) -fno-builtin -g -lg -W -Wreturn-type -Wcomment
# XLIBS = -lX11 -lnls -lnsl_s -lpt
# X_INC_DIR = /usr/X/include
# X_LIB_DIR = /usr/X/lib

# SVr4 using SGCS X11R5
#
# CC = gcc
# CFLAGS = $(INC) -fno-builtin -O -s -W -Wreturn-type -Wcomment
# CFLAGS = $(INC) -fno-builtin -g -lg -W -Wreturn-type -Wcomment
# XLIBS = -lX11 -lsocket -lnsl
# X_INC_DIR = /usr/X/include
# X_LIB_DIR = /usr/X/lib

# Linux using XFree86
#
CC = gcc
#CFLAGS = $(INC) -fno-builtin -O -s -W -Wreturn-type -Wcomment
CFLAGS = $(INC) -fno-builtin -g -W -Wreturn-type -Wcomment
XLIBS = -lX11
X_INC_DIR = /usr/X11/include
X_LIB_DIR = /usr/X11/lib

BINARIES = gif2epsn gif2ps gif2rgb gif2x11 gifasm gifbg \
	gifclip gifclrmp gifcomb giffix gifflip gifhisto \
	gifinter gifinto gifovly gifpos gifrotat gifrsize giftext \
	gifwedge raw2gif rgb2gif text2gif gifspnge giffiltr \
	icon2gif gifcolor

RLE =   gif2rle rle2gif

IRIS =  gif2iris

SCRIPTS = gifcompose gifburst

all: $(BINARIES)
rle: $(RLE)
iris: $(IRIS)

install-all:
	cp $(BINARIES) $(SCRIPTS) $(DEST)

install-rle:
	cp $(RLE) $(DEST)

install-iris:
	cp $(IRIS) $(DEST)

.c:
	$(CC) $(CFLAGS) $< $(GIF_LIB) -lm $(LDFLAGS) -o $*

uninstall-utils:
	(cd $(DEST); rm -f $(BINARIES) $(RLE) $(IRIS) $(SCRIPTS))

gif2iris: gif2iris.c
	$(CC) gif2iris.c $(CFLAGS) $(LDFLAGS) -lgl_s -o gif2iris

gif2x11: gif2x11.c
	$(CC) gif2x11.c $(CFLAGS) -I$(X_INC_DIR) -L$(X_LIB_DIR) $(LDFLAGS) $(GIF_LIB) $(XLIBS) -o gif2x11


clean:
	rm -f $(BINARIES) $(RLE) $(IRIS)
