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

#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "util/autocrop.h"
#include "util/functions.h"

void autocrop_sprite(Sprite *sprite)
{
  if (sprite != NULL) {
    int old_frame = sprite->frame;
    Mask *old_mask = sprite->mask;
    Mask *mask;
    Image *image;
    int x1, y1, x2, y2;
    int u1, v1, u2, v2;

    x1 = y1 = INT_MAX;
    x2 = y2 = INT_MIN;

    image = image_new(sprite->imgtype, sprite->w, sprite->h);
    if (!image)
      return;

    for (sprite->frame=0; sprite->frame<sprite->frames; sprite->frame++) {
      image_clear(image, 0);
      sprite_render(sprite, image, 0, 0);

      /* TODO configurable (what color pixel to use as "refpixel",
	 here we are using the top-left pixel by default) */
      if (get_shrink_rect(&u1, &v1, &u2, &v2,
			  image, image_getpixel (image, 0, 0))) {
	x1 = MIN(x1, u1);
	y1 = MIN(y1, v1);
	x2 = MAX(x2, u2);
	y2 = MAX(y2, v2);
      }
    }
    sprite->frame = old_frame;

    image_free(image);

    /* do nothing */
    if (x1 > x2 || y1 > y2)
      return;

    mask = mask_new();
    mask_replace(mask, x1, y1, x2-x1+1, y2-y1+1);

    sprite->mask = mask;
    CropSprite(sprite);

    sprite->mask = old_mask;
    sprite_generate_mask_boundaries(sprite);
  }
}

bool get_shrink_rect(int *x1, int *y1, int *x2, int *y2,
		     Image *image, int refpixel)
{
#define SHRINK_SIDE(u_begin, u_op, u_final, u_add,		\
		    v_begin, v_op, v_final, v_add, U, V, var)	\
  do {								\
    for (u = u_begin; u u_op u_final; u u_add) {		\
      for (v = v_begin; v v_op v_final; v v_add) {		\
	if (image->method->getpixel (image, U, V) != refpixel)	\
	  break;						\
      }								\
      if (v == v_final)						\
	var;							\
      else							\
	break;							\
    }								\
  } while (0)

  int u, v;

  *x1 = 0;
  *y1 = 0;
  *x2 = image->w-1;
  *y2 = image->h-1;

  SHRINK_SIDE(0, <, image->w, ++,
	      0, <, image->h, ++, u, v, (*x1)++);

  SHRINK_SIDE(0, <, image->h, ++,
	      0, <, image->w, ++, v, u, (*y1)++);

  SHRINK_SIDE(image->w-1, >, 0, --,
	      0, <, image->h, ++, u, v, (*x2)--);

  SHRINK_SIDE(image->h-1, >, 0, --,
	      0, <, image->w, ++, v, u, (*y2)--);

  if ((*x1 > *x2) || (*y1 > *y2))
    return FALSE;
  else
    return TRUE;

#undef SHRINK_SIDE
}

bool get_shrink_rect2(int *x1, int *y1, int *x2, int *y2,
		      Image *image, Image *refimage)
{
#define SHRINK_SIDE(u_begin, u_op, u_final, u_add,		\
		    v_begin, v_op, v_final, v_add, U, V, var)	\
  do {								\
    for (u = u_begin; u u_op u_final; u u_add) {		\
      for (v = v_begin; v v_op v_final; v v_add) {		\
	if (image->method->getpixel (image, U, V) !=		\
	    refimage->method->getpixel (refimage, U, V))	\
	  break;						\
      }								\
      if (v == v_final)						\
	var;							\
      else							\
	break;							\
    }								\
  } while (0)

  int u, v;

  *x1 = 0;
  *y1 = 0;
  *x2 = image->w-1;
  *y2 = image->h-1;

  SHRINK_SIDE(0, <, image->w, ++,
	      0, <, image->h, ++, u, v, (*x1)++);

  SHRINK_SIDE(0, <, image->h, ++,
	      0, <, image->w, ++, v, u, (*y1)++);

  SHRINK_SIDE(image->w-1, >, 0, --,
	      0, <, image->h, ++, u, v, (*x2)--);

  SHRINK_SIDE(image->h-1, >, 0, --,
	      0, <, image->w, ++, v, u, (*y2)--);

  if ((*x1 > *x2) || (*y1 > *y2))
    return FALSE;
  else
    return TRUE;

#undef SHRINK_SIDE
}

