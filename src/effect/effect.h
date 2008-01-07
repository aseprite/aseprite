/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007, 2008  David A. Capello
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

#ifndef EFFECT_H
#define EFFECT_H

#include <stdlib.h>
#include "jinete/jbase.h"

struct Image;
struct Mask;
struct Sprite;

int init_module_effect(void);
void exit_module_effect(void);

typedef struct Effect
{
  struct Sprite *sprite;
  struct Image *src, *dst;
  int row;
  int x, y, w, h;
  int offset_x, offset_y;
  struct Mask *mask;
  struct Mask *preview_mask;
  unsigned char *mask_address;
  div_t d;
  void (*apply)(struct Effect *effect);
  struct {
    int r:1, g:1, b:1;
    int k:1;
    int a:1;
    int index:1;
  } target;
} Effect;

Effect *effect_new(struct Sprite *sprite, const char *name);
void effect_free(Effect *effect);

void effect_load_target(Effect *effect);
void effect_set_target(Effect *effect, bool r, bool g, bool b, bool k, bool a, bool index);
void effect_set_target_rgb(Effect *effect, bool r, bool g, bool b, bool a);
void effect_set_target_grayscale(Effect *effect, bool k, bool a);
void effect_set_target_indexed(Effect *effect, bool r, bool g, bool b, bool index);

void effect_begin(Effect *effect);
void effect_begin_for_preview(Effect *effect);
bool effect_apply_step(Effect *effect);

void effect_apply(Effect *effect);
void effect_flush(Effect *effect);

void effect_apply_to_image(Effect *effect, struct Image *image, int x, int y);
void effect_apply_to_target(Effect *effect);

/* macro to get contiguos pixels from an image */
#define GET_MATRIX_DATA(ptr_type, width, height, cx, cy, do_job)	\
  /* Y position to get pixel */						\
  gety = y-(cy);							\
  addy = 0;								\
  if (gety < 0) {							\
    if (data.tiled)							\
      gety = src->h - (-(gety+1) % src->h) - 1;				\
    else {								\
      addy = -gety; 							\
      gety = 0;								\
    }									\
  }									\
  else if (gety >= src->h) {						\
    if (data.tiled)							\
      gety = gety % src->h;						\
    else								\
      gety = src->h-1;							\
  }									\
									\
  for (dy=0; dy<(height); dy++) {					\
    /* X position to get pixel */					\
    getx = x-(cx);							\
    addx = 0;								\
    if (getx < 0) {							\
      if (data.tiled)							\
	getx = src->w - (-(getx+1) % src->w) - 1;			\
      else {								\
	addx = -getx; 							\
	getx = 0;							\
      }									\
    }									\
    else if (getx >= src->w) {						\
      if (data.tiled)							\
	getx = getx % src->w;						\
      else								\
	getx = src->w-1;						\
    }									\
									\
    src_address = ((ptr_type **)src->line)[gety]+getx;			\
									\
    for (dx=0; dx<width; dx++) {					\
      do_job								\
									\
      /* update X position to get pixel */				\
      if (getx < src->w-1) {						\
	getx++;								\
	if (addx == 0)							\
	  src_address++;						\
	else								\
	  addx--;							\
      }									\
      else if (data.tiled) {						\
	getx = 0;							\
	src_address = ((ptr_type **)src->line)[gety]+getx;		\
      }									\
    }									\
									\
    /* update Y position to get pixel */				\
    if (gety < src->h-1) {						\
      if (addy == 0)							\
	gety++;								\
      else								\
	addy--;								\
    }									\
    else if (data.tiled)						\
      gety = 0;								\
  }

#endif /* EFFECT_H */
