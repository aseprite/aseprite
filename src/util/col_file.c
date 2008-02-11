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

#include <stdio.h>
#include <allegro.h>

#include "file/file.h"
#include "raster/image.h"

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
  FILE *f;

  if (!(size) || (pro && d.rem)) /* invalid format */
    return NULL;

  f = fopen(filename, "rb");
  if (!f)
    return NULL;

  /* Animator format */
  if (!pro) {
    palette = jmalloc(sizeof (PALETTE));

    for (c=0; c<256; c++) {
      r = fgetc(f);
      g = fgetc(f);
      b = fgetc(f);
      palette[c].r = MID(0, r, 63);
      palette[c].g = MID(0, g, 63);
      palette[c].b = MID(0, b, 63);
    }
  }
  /* Animator Pro format */
  else {
    int magic, version;

    fgetl(f);			/* skip file size */
    magic = fgetw(f);		/* file format identifier */
    version = fgetw(f);		/* version file */

    /* unknown format */
    if (magic != 0xB123 || version != 0) {
      fclose (f);
      return NULL;
    }

    palette = jmalloc(sizeof (PALETTE));

    for (c=0; c<d.quot && c<256; c++) {
      r = fgetc(f);
      g = fgetc(f);
      b = fgetc(f);
      palette[c].r = MID(0, r, 255)>>2;
      palette[c].g = MID(0, g, 255)>>2;
      palette[c].b = MID(0, b, 255)>>2;
    }

    for (; c<256; c++)
      palette[c].r = palette[c].g = palette[c].b = 0;
  }

  fclose(f);
  return palette;
}

/* saves an Animator Pro COL file */
int save_col_file(RGB *palette, const char *filename)
{
  FILE *f;
  int c;

  f = fopen(filename, "wb");
  if (!f)
    return -1;

  fputl(8+768, f);		/* file size */
  fputw(0xB123, f);		/* file format identifier */
  fputw(0, f);			/* version file */

  for (c=0; c<256; c++) {
    fputc(_rgb_scale_6[palette[c].r], f);
    fputc(_rgb_scale_6[palette[c].g], f);
    fputc(_rgb_scale_6[palette[c].b], f);
  }

  fclose(f);
  return 0;
}
