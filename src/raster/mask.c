/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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

#include <stdlib.h>
#include <string.h>

#include "raster/image.h"
#include "raster/mask.h"

#endif

static void shrink_mask (Mask *mask);

Mask *mask_new (void)
{
  Mask *mask = (Mask *)gfxobj_new(GFXOBJ_MASK, sizeof(Mask));
  if (!mask)
    return NULL;

  mask->name = NULL;
  mask->x = 0;
  mask->y = 0;
  mask->w = 0;
  mask->h = 0;
  mask->bitmap = NULL;

  return mask;
}

Mask *mask_new_copy (const Mask *mask)
{
  Mask *copy;

  copy = mask_new ();
  if (!copy)
    return NULL;

  mask_copy (copy, mask);
  return copy;
}

void mask_free (Mask *mask)
{
  if (mask->name)
    jfree (mask->name);

  if (mask->bitmap)
    image_free (mask->bitmap);

  gfxobj_free((GfxObj *)mask);
}

int mask_is_empty (Mask *mask)
{
  return (!mask->bitmap)? TRUE: FALSE;
}

void mask_set_name (Mask *mask, const char *name)
{
  if (mask->name)
    jfree (mask->name);

  mask->name = name ? jstrdup (name): NULL;
}

void mask_copy (Mask *mask_dst, const Mask *mask_src)
{
  mask_none (mask_dst);

  if (mask_src->name)
    mask_set_name (mask_dst, mask_src->name);

  if (mask_src->bitmap) {
    /* add all the area of "mask" */
    mask_union (mask_dst, mask_src->x, mask_src->y,
		mask_src->w, mask_src->h);

    /* and copy the "mask" bitmap */
    image_copy (mask_dst->bitmap, mask_src->bitmap, 0, 0);
  }
}

void mask_move (Mask *mask, int x, int y)
{
  mask->x += x;
  mask->y += y;
}

void mask_none (Mask *mask)
{
  if (mask->bitmap) {
    image_free (mask->bitmap);
    mask->bitmap = NULL;
    mask->x = 0;
    mask->y = 0;
    mask->w = 0;
    mask->h = 0;
  }
}

void mask_invert (Mask *mask)
{
  if (mask->bitmap) {
    unsigned char *address;
    int u, v;
    div_t d;

    for (v=0; v<mask->h; v++) {
      d.quot = d.rem = 0;
      address = ((unsigned char **)mask->bitmap->line)[v];
      for (u=0; u<mask->w; u++) {
	*address ^= (1<<d.rem);
	_image_bitmap_next_bit (d, address);
      }
    }

    shrink_mask (mask);
  }
}

void mask_replace (Mask *mask, int x, int y, int w, int h)
{
  mask->x = x;
  mask->y = y;
  mask->w = w;
  mask->h = h;

  if (mask->bitmap)
    image_free (mask->bitmap);

  mask->bitmap = image_new (IMAGE_BITMAP, w, h);
  image_clear (mask->bitmap, 1);
}

void mask_union (Mask *mask, int x, int y, int w, int h)
{
  if (!mask->bitmap) {
    mask->x = x;
    mask->y = y;
    mask->w = w;
    mask->h = h;
    mask->bitmap = image_new (IMAGE_BITMAP, w, h);
  }
  else {
    Image *image;
    int x1 = mask->x;
    int y1 = mask->y;
    int x2 = MAX (mask->x+mask->w-1, x+w-1);
    int y2 = MAX (mask->y+mask->h-1, y+h-1);

    mask->x = MIN (x, x1);
    mask->y = MIN (y, y1);
    mask->w = x2 - mask->x + 1;
    mask->h = y2 - mask->y + 1;

    image = image_crop (mask->bitmap, mask->x-x1, mask->y-y1, mask->w, mask->h);
    image_free (mask->bitmap);
    mask->bitmap = image;
  }

  image_rectfill (mask->bitmap,
		  x-mask->x, y-mask->y,
		  x-mask->x+w-1, y-mask->y+h-1, 1);
}

void mask_subtract (Mask *mask, int x, int y, int w, int h)
{
  if (mask->bitmap) {
    image_rectfill (mask->bitmap,
		    x-mask->x, y-mask->y,
		    x-mask->x+w-1, y-mask->y+h-1, 0);
    shrink_mask (mask);
  }
}

void mask_intersect (Mask *mask, int x, int y, int w, int h)
{
  if (mask->bitmap) {
    Image *image;
    int x1 = mask->x;
    int y1 = mask->y;
    int x2 = MIN (mask->x+mask->w-1, x+w-1);
    int y2 = MIN (mask->y+mask->h-1, y+h-1);

    mask->x = MAX (x, x1);
    mask->y = MAX (y, y1);
    mask->w = x2 - mask->x + 1;
    mask->h = y2 - mask->y + 1;

    image = image_crop (mask->bitmap, mask->x-x1, mask->y-y1, mask->w, mask->h);
    image_free (mask->bitmap);
    mask->bitmap = image;

    shrink_mask (mask);
  }
}

void mask_merge (Mask *mask, const Mask *src)
{
  /* XXXX!!! */
}

void mask_by_color (Mask *mask, const Image *src, int color, int fuzziness)
{
  Image *dst;

  mask_replace (mask, 0, 0, src->w, src->h);

  dst = mask->bitmap;

  switch (src->imgtype) {

    case IMAGE_RGB: {
      unsigned long *src_address;
      unsigned char *dst_address;
      int src_r, src_g, src_b, src_a;
      int dst_r, dst_g, dst_b, dst_a;
      int u, v, c;
      div_t d;

      dst_r = _rgba_getr (color);
      dst_g = _rgba_getg (color);
      dst_b = _rgba_getb (color);
      dst_a = _rgba_geta (color);

      for (v=0; v<src->h; v++) {
	src_address = ((unsigned long **)src->line)[v];
	dst_address = ((unsigned char **)dst->line)[v];

	d = div (0, 8);

	for (u=0; u<src->w; u++) {
	  c = *(src_address++);

	  src_r = _rgba_getr (c);
	  src_g = _rgba_getg (c);
	  src_b = _rgba_getb (c);
	  src_a = _rgba_geta (c);

	  if (!((src_r >= dst_r-fuzziness) && (src_r <= dst_r+fuzziness) &&
		(src_g >= dst_g-fuzziness) && (src_g <= dst_g+fuzziness) &&
		(src_b >= dst_b-fuzziness) && (src_b <= dst_b+fuzziness) &&
		(src_a >= dst_a-fuzziness) && (src_a <= dst_a+fuzziness)))
	    (*dst_address) ^= (1 << d.rem);

	  _image_bitmap_next_bit (d, dst_address);
	}
      }
    } break;

    case IMAGE_GRAYSCALE: {
      unsigned short *src_address;
      unsigned char *dst_address;
      int src_k, src_a;
      int dst_k, dst_a;
      int u, v, c;
      div_t d;

      dst_k = _graya_getk (color);
      dst_a = _graya_geta (color);

      for (v=0; v<src->h; v++) {
	src_address = ((unsigned short **)src->line)[v];
	dst_address = ((unsigned char **)dst->line)[v];

	d = div (0, 8);

	for (u=0; u<src->w; u++) {
	  c = *(src_address++);

	  src_k = _graya_getk (c);
	  src_a = _graya_geta (c);

	  if (!((src_k >= dst_k-fuzziness) && (src_k <= dst_k+fuzziness) &&
		(src_a >= dst_a-fuzziness) && (src_a <= dst_a+fuzziness)))
	    (*dst_address) ^= (1 << d.rem);

	  _image_bitmap_next_bit (d, dst_address);
	}
      }
    } break;

    case IMAGE_INDEXED: {
      unsigned char *src_address;
      unsigned char *dst_address;
      int u, v, c;
      div_t d;

      for (v=0; v<src->h; v++) {
	src_address = ((unsigned char **)src->line)[v];
	dst_address = ((unsigned char **)dst->line)[v];

	d = div (0, 8);

	for (u=0; u<src->w; u++) {
	  c = *(src_address++);

	  if (!((c >= color-fuzziness) && (c <= color+fuzziness)))
	    (*dst_address) ^= (1 << d.rem);

	  _image_bitmap_next_bit (d, dst_address);
	}
      }
    } break;
  }

  shrink_mask (mask);
}

/* void mask_fill (Mask *mask, Image *image, int color) */
/* { */
/*   if (mask) { */
/*     unsigned char *address; */
/*     int u, v; */
/*     div_t d; */

/*     for (v=0; v<h; v++) { */
/*       d = div (0, 8); */
/*       address = ((unsigned char **)drawable->lines)[v]+d.quot; */

/*       for (u=0; u<w; u++) { */
/* 	if ((*address) & (1<<d.rem)) */
/* 	  drawable->putpixel (x+u, y+v, color); */

/* 	_drawable_bitmap_next_bit (d, address); */
/*       } */
/*     } */
/*   } */
/* } */

void mask_crop (Mask *mask, const Image *image)
{
#define ADVANCE(beg, end, o_end, cmp, op, getpixel1, getpixel)	\
  {								\
    done = TRUE;						\
    for (beg=beg_##beg; beg cmp beg_##end; beg op) {		\
      old_color = getpixel1;					\
      done = TRUE;						\
      for (c++; c<=beg_##o_end; c++) {				\
	if (getpixel != old_color) {				\
	  done = FALSE;						\
	  break;						\
	}							\
      }								\
      if (!done)						\
	break;							\
    }								\
    if (done)							\
      done_count++;						\
  }

  int beg_x1, beg_y1, beg_x2, beg_y2;
  int c, x1, y1, x2, y2, old_color;
  int done_count = 0;
  int done;

  if (!mask->bitmap)
    return;

  beg_x1 = mask->x;
  beg_y1 = mask->y;
  beg_x2 = beg_x1 + mask->w - 1;
  beg_y2 = beg_y1 + mask->h - 1;

  beg_x1 = MID (0, beg_x1, mask->w-1);
  beg_y1 = MID (0, beg_y1, mask->h-1);
  beg_x2 = MID (beg_x1, beg_x2, mask->w-1);
  beg_y2 = MID (beg_y1, beg_y2, mask->h-1);

  /* left */
  ADVANCE (x1, x2, y2, <=, ++,
	   image_getpixel (image, x1, c=beg_y1),
	   image_getpixel (image, x1, c));
  /* right */
  ADVANCE (x2, x1, y2, >=, --,
	   image_getpixel (image, x2, c=beg_y1),
	   image_getpixel (image, x2, c));
  /* top */
  ADVANCE (y1, y2, x2, <=, ++,
	   image_getpixel (image, c=beg_x1, y1),
	   image_getpixel (image, c, y1));
  /* bottom */
  ADVANCE (y2, y1, x2, >=, --,
	   image_getpixel (image, c=beg_x1, y2),
	   image_getpixel (image, c, y2));

  if (done_count < 4)
    mask_intersect (mask, x1, y1, x2, y2);
  else
    mask_none (mask);

#undef ADVANCE
}

static void shrink_mask (Mask *mask)
{
#define SHRINK_SIDE(u_begin, u_op, u_final, u_add,			\
		    v_begin, v_op, v_final, v_add, U, V, var)		\
  {									\
    for (u = u_begin; u u_op u_final; u u_add) {			\
      for (v = v_begin; v v_op v_final; v v_add) {			\
	if (mask->bitmap->method->getpixel (mask->bitmap, U, V))	\
	  break;							\
      }									\
      if (v == v_final)							\
	var;								\
      else								\
	break;								\
    }									\
  }

  int u, v, x1, y1, x2, y2;

  x1 = mask->x;
  y1 = mask->y;
  x2 = mask->x+mask->w-1;
  y2 = mask->y+mask->h-1;

  SHRINK_SIDE (0, <, mask->w, ++,
	       0, <, mask->h, ++, u, v, x1++);

  SHRINK_SIDE (0, <, mask->h, ++,
	       0, <, mask->w, ++, v, u, y1++);

  SHRINK_SIDE (mask->w-1, >, 0, --,
	       0, <, mask->h, ++, u, v, x2--);

  SHRINK_SIDE (mask->h-1, >, 0, --,
	       0, <, mask->w, ++, v, u, y2--);

  if ((x1 == x2) && (y1 == y2)) {
    mask_none (mask);
  }
  else if ((x1 != mask->x) || (x2 != mask->x+mask->w-1) ||
	   (y1 != mask->y) || (y2 != mask->y+mask->h-1)) {
    Image *image;

    u = mask->x;
    v = mask->y;

    mask->x = x1;
    mask->y = y1;
    mask->w = x2 - x1 + 1;
    mask->h = y2 - y1 + 1;

    image = image_crop (mask->bitmap, mask->x-u, mask->y-v, mask->w, mask->h);
    image_free (mask->bitmap);
    mask->bitmap = image;
  }

#undef SHRINK_SIDE
}
