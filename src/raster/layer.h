/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#ifndef RASTER_LAYER_H
#define RASTER_LAYER_H

#include "jinete/jbase.h"
#include "raster/gfxobj.h"

class Cel;
class Image;
class Sprite;

#define LAYER_NAME_SIZE		256

#define LAYER_IS_READABLE	0x0001
#define LAYER_IS_WRITABLE	0x0002
#define LAYER_IS_LOCKMOVE	0x0004
#define LAYER_IS_BACKGROUND	0x0008

class Layer : public GfxObj
{
public:
  char name[LAYER_NAME_SIZE];	/* layer name */
  Sprite* sprite;		/* owner of the layer */
  Layer* parent_layer;		/* parent layer */
  unsigned short flags;

  /* for GFXOBJ_LAYER_IMAGE */
  int blend_mode;		/* constant blend mode */
  JList cels;			/* list of cels */

  /* for GFXOBJ_LAYER_SET */
  JList layers;

  Layer(int type, Sprite* sprite);
  virtual ~Layer();
};

Layer* layer_new(Sprite* sprite);
Layer* layer_set_new(Sprite* sprite);
Layer* layer_new_copy(Sprite* dst_sprite, const Layer* src_layer);
Layer* layer_new_flatten_copy(Sprite* dst_sprite, const Layer* src_layer,
			      int x, int y, int w, int h, int frmin, int frmax);
void layer_free(Layer* layer);
void layer_free_images(Layer* layer);

void layer_configure_as_background(Layer* layer);

bool layer_is_image(const Layer* layer);
bool layer_is_set(const Layer* layer);
bool layer_is_background(const Layer* layer);
bool layer_is_moveable(const Layer* layer);
bool layer_is_readable(const Layer* layer);
bool layer_is_writable(const Layer* layer);

Layer* layer_get_prev(Layer* layer);
Layer* layer_get_next(Layer* layer);

void layer_set_name(Layer* layer, const char *name);
void layer_set_blend_mode(Layer* layer, int blend_mode);
void layer_get_cels(const Layer* layer, JList cels);

/* for LAYER_IMAGE */
void layer_add_cel(Layer* layer, Cel *cel);
void layer_remove_cel(Layer* layer, Cel *cel);
Cel *layer_get_cel(const Layer* layer, int frame);

/* for LAYER_SET */
void layer_add_layer(Layer* set, Layer* layer);
void layer_remove_layer(Layer* set, Layer* layer);
void layer_move_layer(Layer* set, Layer* layer, Layer* after);

void layer_render(const Layer* layer, Image *image, int x, int y, int frame);

#endif /* RASTER_LAYER_H */
