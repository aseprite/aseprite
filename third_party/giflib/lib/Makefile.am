## Process this file with automake to produce a Makefile.in for libungif.

EXTRA_DIST = Makefile.unx

lib_LTLIBRARIES = libgif.la
noinst_LIBRARIES = libgetarg.a

include_HEADERS = gif_lib.h

libgetarg_a_SOURCES = getarg.c getarg.h

libgif_la_SOURCES = dev2gif.c   \
                    dgif_lib.c  \
                    egif_lib.c  \
                    gif_err.c   \
                    gif_font.c  \
					gif_hash.c  \
					gif_hash.h	\
                    gifalloc.c  \
                    qprintf.c   \
                    quantize.c  \
					gif_lib_private.h

libgif_la_LDFLAGS = -version-info 5:6:1
libgif_la_LIBADD = @DEVS@
libgif_CFLAGS = $(X_CFLAGS) $(AM_CFLAGS)
