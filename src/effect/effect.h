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

#ifndef EFFECT_EFFECT_H_INCLUDED
#define EFFECT_EFFECT_H_INCLUDED

#include <stdlib.h>
#include <cstring>
#include "sprite_wrappers.h"
#include "jinete/jbase.h"
#include "tiled_mode.h"

class Image;
class Mask;
class Sprite;

#define TARGET_RED_CHANNEL		1
#define TARGET_GREEN_CHANNEL		2
#define TARGET_BLUE_CHANNEL		4
#define TARGET_ALPHA_CHANNEL		8
#define TARGET_GRAY_CHANNEL		16
#define TARGET_INDEX_CHANNEL		32
#define TARGET_ALL_FRAMES		64
#define TARGET_ALL_LAYERS		128

#define TARGET_ALL_CHANNELS		\
  (TARGET_RED_CHANNEL		|	\
   TARGET_GREEN_CHANNEL		|	\
   TARGET_BLUE_CHANNEL		|	\
   TARGET_ALPHA_CHANNEL		|	\
   TARGET_GRAY_CHANNEL		)


class invalid_effect_exception : public ase_exception
{
public:
  invalid_effect_exception(const char* effect_name) throw()
    : ase_exception("Invalid effect specified: %s", effect_name) { }
};

class invalid_imgtype_exception : public ase_exception
{
public:
  invalid_imgtype_exception() throw()
  : ase_exception("Invalid image type specified.") { }
};

class invalid_area_exception : public ase_exception
{
public:
  invalid_area_exception() throw()
  : ase_exception("The current mask/area to apply the effect is completelly invalid.") { }
};

class no_image_exception : public ase_exception
{
public:
  no_image_exception() throw()
  : ase_exception("There are not an active image to apply the effect.\n"
		  "Please select a layer/cel with an image and try again.") { }
};

struct EffectData;

struct Effect
{
  SpriteWriter sprite;
  Image* src;
  Image* dst;
  int row;
  int x, y, w, h;
  int offset_x, offset_y;
  Mask* mask;
  Mask* preview_mask;
  unsigned char *mask_address;
  div_t d;
  struct EffectData *effect_data;
  void (*apply)(struct Effect* effect);
  int _target;			/* original targets */
  int target;			/* filtered targets */
  /* hooks */
  float progress_base, progress_width;
  void *progress_data;
  void (*progress)(void *data, float progress);
  bool (*is_cancelled)(void *data);

  Effect(const SpriteReader& sprite, const char* name);
  ~Effect();
};

int init_module_effect();
void exit_module_effect();

void effect_set_target(Effect* effect, int target);

void effect_begin(Effect* effect);
void effect_begin_for_preview(Effect* effect);
bool effect_apply_step(Effect* effect);

void effect_apply(Effect* effect);
void effect_flush(Effect* effect);

void effect_apply_to_target(Effect* effect);

/**
 * Macro to get contiguos pixels from an image. It's useful to fill a
 * matrix of pixels of "width x height" dimension.
 *
 * It needs some variables to be defined before to use this routines:
 * x, y = pixel position
 * getx, gety, addx, addy, dx, dy = auxiliars
 */
#define GET_MATRIX_DATA(ptr_type, src, src_address,			\
			width, height, cx, cy, tiled, do_job)		\
  /* Y position to get pixel */						\
  gety = y-(cy);							\
  addy = 0;								\
  if (gety < 0) {							\
    if (tiled & TILED_Y_AXIS)						\
      gety = src->h - (-(gety+1) % src->h) - 1;				\
    else {								\
      addy = -gety; 							\
      gety = 0;								\
    }									\
  }									\
  else if (gety >= src->h) {						\
    if (tiled & TILED_Y_AXIS)						\
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
      if (tiled & TILED_X_AXIS)						\
	getx = src->w - (-(getx+1) % src->w) - 1;			\
      else {								\
	addx = -getx; 							\
	getx = 0;							\
      }									\
    }									\
    else if (getx >= src->w) {						\
      if (tiled & TILED_X_AXIS)						\
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
      else if (tiled & TILED_X_AXIS) {					\
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
    else if (tiled & TILED_Y_AXIS)					\
      gety = 0;								\
  }

#endif
