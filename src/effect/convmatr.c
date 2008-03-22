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

#include <allegro.h>
#include <stdio.h>
#include <string.h>

#include "jinete/jlist.h"

#include "core/cfg.h"
#include "core/dirs.h"
#include "effect/convmatr.h"
#include "effect/effect.h"
#include "modules/palettes.h"
#include "modules/tools.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "util/filetoks.h"

/* TODO warning: this number could be dangerous for big filters */
#define PRECISION          (256)

static struct {
  JList matrices;
  ConvMatr *convmatr;
  int tiled;
  unsigned char **lines;
} data;

void init_convolution_matrix(void)
{
  data.matrices = jlist_new();
  data.convmatr = NULL;
  data.tiled = FALSE;
  data.lines = NULL;

  reload_matrices_stock();
}

void exit_convolution_matrix(void)
{
  clean_matrices_stock();
  jlist_free(data.matrices);

  if (data.lines != NULL)
    jfree(data.lines);
}

ConvMatr *convmatr_new(int w, int h)
{
  ConvMatr *convmatr;
  int c, size;

  convmatr = (ConvMatr *)jnew(ConvMatr, 1);
  if (!convmatr)
    return NULL;

  convmatr->name = NULL;
  convmatr->w = w;
  convmatr->h = h;
  convmatr->cx = convmatr->w/2;
  convmatr->cy = convmatr->h/2;

  size = convmatr->w * convmatr->h;

  convmatr->data = jmalloc(sizeof(int) * size);
  if (!convmatr->data) {
    convmatr_free(convmatr);
    return NULL;
  }

  for (c=0; c<size; c++)
    convmatr->data[c] = 0;

  convmatr->div = PRECISION;
  convmatr->bias = 0;

  return convmatr;
}

ConvMatr *convmatr_new_string(const char *format)
{
  /* TODO */
  return NULL;
}

void convmatr_free(ConvMatr *convmatr)
{
  if (convmatr->name)
    jfree(convmatr->name);
    
  if (convmatr->data)
    jfree(convmatr->data);

  jfree(convmatr);
}

void set_convmatr(ConvMatr *convmatr)
{
  data.convmatr = convmatr;
  data.tiled = get_tiled_mode();

  if (data.lines != NULL)
    jfree(data.lines);

  data.lines = jmalloc(sizeof(unsigned char *) * convmatr->h);
}

ConvMatr *get_convmatr(void)
{
  return data.convmatr;
}

ConvMatr *get_convmatr_by_name(const char *name)
{
  ConvMatr *convmatr;
  JLink link;

  JI_LIST_FOR_EACH(data.matrices, link) {
    convmatr = link->data;
    if (strcmp(convmatr->name, name) == 0)
      return convmatr;
  }

  return NULL;
}

void reload_matrices_stock(void)
{
#define READ_TOK() {					\
    if (!tok_read(f, buf, leavings, sizeof (leavings)))	\
      break;						\
  }

#define READ_INT(var) {				\
    READ_TOK();					\
    var = ustrtol(buf, NULL, 10);		\
  }

  const char *names[] = { "convmatr.usr",
			  "convmatr.gen",
			  "convmatr.def", NULL };
  char *s, buf[256], leavings[4096];
  int i, c, w, h, div, bias;
  DIRS *dirs, *dir;
  ConvMatr *convmatr;
  FILE *f;
  char *name;

  clean_matrices_stock();

  for (i=0; names[i]; i++) {
    dirs = filename_in_datadir(names[i]);

    for (dir=dirs; dir; dir=dir->next) {
      /* open matrices stock file */
      f = fopen(dir->path, "r");
      if (!f)
	continue;

      tok_reset_line_num();

      name = NULL;
      convmatr = NULL;
      strcpy(leavings, "");

      /* read the matrix name */
      while (tok_read (f, buf, leavings, sizeof (leavings))) {
	/* name of the matrix */
	name = jstrdup(buf);

	/* width and height */
	READ_INT(w);
	READ_INT(h);

	if ((w <= 0) || (w > 32) ||
	    (h <= 0) || (h > 32))
	  break;

	/* create the matrix data */
	convmatr = convmatr_new(w, h);
	if (!convmatr)
	  break;

	/* centre */
	READ_INT(convmatr->cx);
	READ_INT(convmatr->cy);

	if ((convmatr->cx < 0) || (convmatr->cx >= w) ||
	    (convmatr->cy < 0) || (convmatr->cy >= h))
	  break;

	/* data */
	READ_TOK();                    /* jump the `{' char */
	if (*buf != '{')
	  break;

	c = 0;
	div = 0;
	for (c=0; c<w*h; c++) {
	  READ_TOK();
	  convmatr->data[c] = ustrtod(buf, NULL) * PRECISION;
	  div += convmatr->data[c];
	}

	READ_TOK();                    /* jump the `}' char */
	if (*buf != '}')
	  break;

	if (div > 0)
	  bias = 0;
	else if (div == 0) {
	  div = PRECISION;
	  bias = 128;
	}
	else {
	  div = ABS(div);
	  bias = 255;
	}

	/* div */
	READ_TOK();
	if (ustricmp (buf, "auto") != 0)
	  div = ustrtod(buf, NULL) * PRECISION;

	convmatr->div = div;

	/* bias */
	READ_TOK();
	if (ustricmp (buf, "auto") != 0)
	  bias = ustrtod(buf, NULL);

	convmatr->bias = bias;

	/* target */
	READ_TOK();

	for (s=buf; *s; s++) {
	  switch (*s) {
	    case 'r': convmatr->default_target |= CONVMATR_R; break;
	    case 'g': convmatr->default_target |= CONVMATR_G; break;
	    case 'b': convmatr->default_target |= CONVMATR_B; break;
	    case 'a': convmatr->default_target |= CONVMATR_A; break;
	  }
	}

	/* name */
	convmatr->name = name;

	/* insert the new matrix in the list */
	jlist_append(data.matrices, convmatr);

	name = NULL;
	convmatr = NULL;
      }

      /* destroy the last invalid matrix in case of error */
      if (name)
	jfree(name);

      if (convmatr)
	convmatr_free(convmatr);

      /* close the file */
      fclose(f);
    }

    dirs_free(dirs);
  }
}

void clean_matrices_stock(void)
{
  JLink link;

  JI_LIST_FOR_EACH(data.matrices, link)
    convmatr_free(link->data);

  jlist_clear(data.matrices);
}

JList get_convmatr_stock(void)
{
  return data.matrices;
}

#define GET_CONVMATR_DATA(ptr_type, do_job)	\
  div = matrix->div;				\
  mdata = matrix->data;				\
						\
  GET_MATRIX_DATA				\
  (ptr_type,					\
   matrix->w, matrix->h,			\
   matrix->cx, matrix->cy,			\
   if (*mdata) {				\
     color = *src_address;			\
     do_job;					\
   }						\
   mdata++;					\
   );						\
						\
  color = src->method->getpixel(src, x, y);	\
  if (div == 0) {				\
    *(dst_address++) = color;			\
    continue;					\
  }

void apply_convolution_matrix4(Effect *effect)
{
  ConvMatr *matrix = data.convmatr;
  Image *src = effect->src;
  Image *dst = effect->dst;
  ase_uint32 *src_address;
  ase_uint32 *dst_address;
  int x, y, dx, dy, color;
  int getx, gety, addx, addy;
  int r, g, b, a;
  int *mdata;
  int w, div;

  if (matrix) {
    w = effect->x+effect->w;
    y = effect->y+effect->row;
    dst_address = ((ase_uint32 **)dst->line)[y]+effect->x;

    for (x=effect->x; x<w; x++) {
      /* avoid the unmask region */
      if (effect->mask_address) {
	if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	  dst_address++;
	  _image_bitmap_next_bit(effect->d, effect->mask_address);
	  continue;
	}
	else
	  _image_bitmap_next_bit(effect->d, effect->mask_address);
      }

      r = g = b = a = 0;

      GET_CONVMATR_DATA
	(ase_uint32,
	 if (_rgba_geta(color) == 0)
	   div -= *mdata;
	 else {
	   r += _rgba_getr(color) * (*mdata);
	   g += _rgba_getg(color) * (*mdata);
	   b += _rgba_getb(color) * (*mdata);
	   a += _rgba_geta(color) * (*mdata);
	 }
	 );

      if (effect->target.r) {
	r = r / div + matrix->bias;
	r = MID(0, r, 255);
      }
      else
	r = _rgba_getr(color);

      if (effect->target.g) {
	g = g / div + matrix->bias;
	g = MID(0, g, 255);
      }
      else
	g = _rgba_getg(color);

      if (effect->target.b) {
	b = b / div + matrix->bias;
	b = MID(0, b, 255);
      }
      else
	b = _rgba_getb(color);

      if (effect->target.a) {
	a = a / matrix->div + matrix->bias;
	a = MID(0, a, 255);
      }
      else
	a = _rgba_geta(color);

      *(dst_address++) = _rgba(r, g, b, a);
    }
  }
}

void apply_convolution_matrix2(Effect *effect)
{
  ConvMatr *matrix = data.convmatr;
  Image *src = effect->src;
  Image *dst = effect->dst;
  ase_uint16 *src_address;
  ase_uint16 *dst_address;
  int x, y, dx, dy, color;
  int getx, gety, addx, addy;
  int k, a;
  int *mdata;
  int w, div;

  if (matrix) {
    w = effect->x+effect->w;
    y = effect->y+effect->row;
    dst_address = ((ase_uint16 **)dst->line)[y]+effect->x;

    for (x=effect->x; x<w; x++) {
      /* avoid the unmask region */
      if (effect->mask_address) {
	if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	  dst_address++;
	  _image_bitmap_next_bit(effect->d, effect->mask_address);
	  continue;
	}
	else
	  _image_bitmap_next_bit(effect->d, effect->mask_address);
      }

      k = a = 0;

      GET_CONVMATR_DATA
	(ase_uint16,
	 if (_graya_geta(color) == 0)
	   div -= *mdata;
	 else {
	   k += _graya_getv(color) * (*mdata);
	   a += _graya_geta(color) * (*mdata);
	 }
	 );

      if (effect->target.k) {
	k = k / div + matrix->bias;
	k = MID(0, k, 255);
      }
      else
	k = _graya_getv(color);

      if (effect->target.a) {
	a = a / matrix->div + matrix->bias;
	a = MID(0, a, 255);
      }
      else
	a = _graya_geta(color);

      *(dst_address++) = _graya(k, a);
    }
  }
}

void apply_convolution_matrix1(Effect *effect)
{
  Palette *pal = get_current_palette();
  ConvMatr *matrix = data.convmatr;
  Image *src = effect->src;
  Image *dst = effect->dst;
  ase_uint8 *src_address;
  ase_uint8 *dst_address;
  int x, y, dx, dy, color;
  int getx, gety, addx, addy;
  int r, g, b, index;
  int *mdata;
  int w, div;

  if (matrix) {
    w = effect->x+effect->w;
    y = effect->y+effect->row;
    dst_address = ((ase_uint8 **)dst->line)[y]+effect->x;

    for (x=effect->x; x<w; x++) {
      /* avoid the unmask region */
      if (effect->mask_address) {
	if (!((*effect->mask_address) & (1<<effect->d.rem))) {
	  dst_address++;
	  _image_bitmap_next_bit(effect->d, effect->mask_address);
	  continue;
	}
	else
	  _image_bitmap_next_bit(effect->d, effect->mask_address);
      }

      r = g = b = index = 0;

      GET_CONVMATR_DATA
	(ase_uint8,
	 r += _rgba_getr(pal->color[color]) * (*mdata);
	 g += _rgba_getg(pal->color[color]) * (*mdata);
	 b += _rgba_getb(pal->color[color]) * (*mdata);
	 index += color * (*mdata);
	 );

      if (effect->target.index) {
	index = index / matrix->div + matrix->bias;
	index = MID(0, index, 255);

	*(dst_address++) = index;
      }
      else {
	if (effect->target.r) {
	  r = r / div + matrix->bias;
	  r = MID(0, r, 255);
	}
	else
	  r = _rgba_getr(pal->color[color]);

	if (effect->target.g) {
	  g = g / div + matrix->bias;
	  g = MID(0, g, 255);
	}
	else
	  g = _rgba_getg(pal->color[color]);

	if (effect->target.b) {
	  b = b / div + matrix->bias;
	  b = MID(0, b, 255);
	}
	else
	  b = _rgba_getb(pal->color[color]);

	*(dst_address++) = orig_rgb_map->data[r>>3][g>>3][b>>3];
      }
    }
  }
}
