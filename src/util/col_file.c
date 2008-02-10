/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>

#include "raster/image.h"

#endif

/* loads a COL file (Animator and Animator Pro format) */
RGB *load_col_file(const char *filename)
{
#if (MAKE_VERSION(4, 2, 1) >= MAKE_VERSION(ALLEGRO_VERSION,		\
					   ALLEGRO_SUB_VERSION,		\
					   ALLEGRO_WIP_VERSION))
  int size = file_size(filename);
#else
  int size = file_size_ex(filename);
#endif
  int pro = (size == 768)? FALSE: TRUE;	/* is Animator Pro format? */
  div_t d = div(size-8, 3);
  RGB *palette = NULL;
  int c, r, g, b;
  PACKFILE *f;

  if (!(size) || (pro && d.rem)) /* invalid format */
    return NULL;

  f = pack_fopen(filename, F_READ);
  if (!f)
    return NULL;

  /* Animator format */
  if (!pro) {
    palette = jmalloc(sizeof (PALETTE));

    for (c=0; c<256; c++) {
      r = pack_getc(f);
      g = pack_getc(f);
      b = pack_getc(f);
      palette[c].r = MID(0, r, 63);
      palette[c].g = MID(0, g, 63);
      palette[c].b = MID(0, b, 63);
    }
  }
  /* Animator Pro format */
  else {
    int magic, version;

    pack_igetl(f);		/* skip file size */
    magic = pack_igetw(f);	/* file format identifier */
    version = pack_igetw(f);	/* version file */

    /* unknown format */
    if (magic != 0xB123 || version != 0) {
      pack_fclose (f);
      return NULL;
    }

    palette = jmalloc(sizeof (PALETTE));

    for (c=0; c<d.quot && c<256; c++) {
      r = pack_getc(f);
      g = pack_getc(f);
      b = pack_getc(f);
      palette[c].r = MID(0, r, 255)>>2;
      palette[c].g = MID(0, g, 255)>>2;
      palette[c].b = MID(0, b, 255)>>2;
    }

    for (; c<256; c++)
      palette[c].r = palette[c].g = palette[c].b = 0;
  }

  pack_fclose(f);
  return palette;
}

/* saves an Animator Pro COL file */
int save_col_file(RGB *palette, const char *filename)
{
  PACKFILE *f;
  int c;

  f = pack_fopen(filename, F_WRITE);
  if (!f)
    return -1;

  pack_iputl(8+768, f);		/* file size */
  pack_iputw(0xB123, f);	/* file format identifier */
  pack_iputw(0, f);		/* version file */

  for (c=0; c<256; c++) {
    pack_putc(_rgb_scale_6[palette[c].r], f);
    pack_putc(_rgb_scale_6[palette[c].g], f);
    pack_putc(_rgb_scale_6[palette[c].b], f);
  }

  pack_fclose(f);
  return 0;
}
