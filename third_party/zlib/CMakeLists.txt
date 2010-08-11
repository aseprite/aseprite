# ASE - Allegro Sprite Editor
# Copyright (C) 2001-2010  David Capello

add_library(zlib
  adler32.c
  compress.c
  crc32.c
  deflate.c
  gzio.c
  infback.c
  inffast.c
  inflate.c
  inftrees.c
  trees.c
  uncompr.c
  zutil.c)
