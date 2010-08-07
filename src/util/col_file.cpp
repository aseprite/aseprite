/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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
#include "raster/palette.h"

#define PROCOL_MAGIC_NUMBER	0xB123

/* loads a COL file (Animator and Animator Pro format) */
Palette *load_col_file(const char *filename)
{
#if (MAKE_VERSION(4, 2, 1) >= MAKE_VERSION(ALLEGRO_VERSION,		\
					   ALLEGRO_SUB_VERSION,		\
					   ALLEGRO_WIP_VERSION))
  int size = file_size(filename);
#else
  int size = file_size_ex(filename);
#endif
  int pro = (size == 768)? false: true;	/* is Animator Pro format? */
  div_t d = div(size-8, 3);
  Palette *pal = NULL;
  int c, r, g, b;
  FILE *f;

  if (!(size) || (pro && d.rem)) /* invalid format */
    return NULL;

  f = fopen(filename, "rb");
  if (!f)
    return NULL;

  /* Animator format */
  if (!pro) {
    pal = new Palette(0, 256);

    for (c=0; c<256; c++) {
      r = fgetc(f);
      g = fgetc(f);
      b = fgetc(f);
      if (ferror(f))
	break;

      pal->setEntry(c, _rgba(_rgb_scale_6[MID(0, r, 63)],
			     _rgb_scale_6[MID(0, g, 63)],
			     _rgb_scale_6[MID(0, b, 63)], 255));
    }
  }
  /* Animator Pro format */
  else {
    int magic, version;

    fgetl(f);			/* skip file size */
    magic = fgetw(f);		/* file format identifier */
    version = fgetw(f);		/* version file */

    /* unknown format */
    if (magic != PROCOL_MAGIC_NUMBER || version != 0) {
      fclose(f);
      return NULL;
    }

    pal = new Palette(0, MIN(d.quot, 256));

    for (c=0; c<pal->size(); c++) {
      r = fgetc(f);
      g = fgetc(f);
      b = fgetc(f);
      if (ferror(f))
	break;

      pal->setEntry(c, _rgba(MID(0, r, 255),
			     MID(0, g, 255),
			     MID(0, b, 255), 255));
    }
  }

  fclose(f);
  return pal;
}

/* saves an Animator Pro COL file */
bool save_col_file(const Palette *pal, const char *filename)
{
  FILE *f = fopen(filename, "wb");
  if (!f)
    return false;

  fputl(8+768, f);		   /* file size */
  fputw(PROCOL_MAGIC_NUMBER, f);   /* file format identifier */
  fputw(0, f);			   /* version file */

  ase_uint32 c;
  for (int i=0; i<256; i++) {
    c = pal->getEntry(i);

    fputc(_rgba_getr(c), f);
    fputc(_rgba_getg(c), f);
    fputc(_rgba_getb(c), f);
    if (ferror(f))
      break;
  }

  fclose(f);
  return true;
}
