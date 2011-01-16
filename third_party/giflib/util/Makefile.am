
bin_PROGRAMS = gif2epsn gif2ps gif2rgb gifasm gifbg gifclip gifcomb \
    giffix gifflip gifhisto gifinter gifinto gifovly gifpos gifrsize \
    giftext gifwedge raw2gif rgb2gif text2gif gifspnge giffiltr \
    icon2gif gifcolor gifinfo @COMPILABLE_EXTRAS@
bin_SCRIPTS = gifcompose gifburst
EXTRA_PROGRAMS = gif2rle rle2gif gif2iris gif2x11 gifclrmp gifrotat

EXTRA_DIST=Makefile.unx $(bin_SCRIPTS)

LDADD = ../lib/libgif.la ../lib/libgetarg.a

AM_CPPFLAGS = -I$(srcdir) -I$(top_srcdir)/lib

gif2rle_SOURCES = gif2rle.c
gif2rle_LDADD = $(LDADD) @RLE_LIB@

rle2gif_SOURCES = rle2gif.c
rle2gif_LDADD = $(LDADD) @RLE_LIB@

gif2iris_SOURCES = gif2iris.c
gif2iris_LDADD = $(LDADD) @GL_S_LIB@

gif2x11_SOURCES = gif2x11.c
gif2x11_LDADD = $(LDADD) @X11_LIB@
gif2x11_CFLAGS = @X_CFLAGS@ $(AM_CFLAGS)

gifclrmp_SOURCES = gifclrmp.c
gifclrmp_LDADD = $(LDADD) @MATH_LIB@

gifrotat_SOURCES = gifrotat.c
gifrotat_LDADD = $(LDADD) @MATH_LIB@

gifinfo_SOURCES = gifinfo.c
gif2epsn_SOURCES = gif2epsn.c
gif2ps_SOURCES = gif2ps.c
gif2rgb_SOURCES = gif2rgb.c
gifasm_SOURCES = gifasm.c
gifbg_SOURCES = gifbg.c
gifclip_SOURCES = gifclip.c
gifcomb_SOURCES = gifcomb.c
giffix_SOURCES = giffix.c
gifflip_SOURCES = gifflip.c
gifhisto_SOURCES = gifhisto.c
gifinter_SOURCES = gifinter.c
gifinto_SOURCES = gifinto.c
gifovly_SOURCES =  gifovly.c
gifpos_SOURCES = gifpos.c
gifrsize_SOURCES = gifrsize.c
giftext_SOURCES = giftext.c
gifwedge_SOURCES = gifwedge.c
raw2gif_SOURCES = raw2gif.c
rgb2gif_SOURCES = rgb2gif.c
text2gif_SOURCES = text2gif.c
gifspnge_SOURCES = gifspnge.c
giffiltr_SOURCES = giffiltr.c
icon2gif_SOURCES = icon2gif.c
gifcolor_SOURCES = gifcolor.c
