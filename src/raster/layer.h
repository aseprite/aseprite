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

#ifndef RASTER_LAYER_H
#define RASTER_LAYER_H

#include "jinete/jbase.h"
#include "raster/gfxobj.h"

struct Cel;
struct Image;
struct Sprite;

typedef struct Layer Layer;

struct Layer
{
  GfxObj gfxobj;
  char name[256];		/* layer name */
  struct Sprite *sprite;	/* owner of the layer */
  GfxObj *parent;		/* parent object */
  unsigned readable : 1;
  unsigned writable : 1;

  /* for GFXOBJ_LAYER_IMAGE */
  int blend_mode;		/* constant blend mode */
  JList cels;			/* list of cels */

  /* for GFXOBJ_LAYER_SET */
  JList layers;
};

Layer *layer_new(struct Sprite *sprite);
Layer *layer_set_new(struct Sprite *sprite);
Layer *layer_new_copy(const Layer *layer);
void layer_free(Layer *layer);

int layer_is_image(const Layer *layer);
int layer_is_set(const Layer *layer);

bool layer_is_readable(const Layer *layer);
bool layer_is_writable(const Layer *layer);

Layer *layer_get_prev(Layer *layer);
Layer *layer_get_next(Layer *layer);

void layer_set_name(Layer *layer, const char *name);
void layer_set_blend_mode(Layer *layer, int blend_mode);
void layer_set_parent(Layer *layer, GfxObj *gfxobj);

/* for LAYER_IMAGE */
void layer_add_cel(Layer *layer, struct Cel *cel);
void layer_remove_cel(Layer *layer, struct Cel *cel);
struct Cel *layer_get_cel(Layer *layer, int frame);

/* for LAYER_SET */
void layer_add_layer(Layer *set, Layer *layer);
void layer_remove_layer(Layer *set, Layer *layer);
void layer_move_layer(Layer *set, Layer *layer, Layer *after);

void layer_render(Layer *layer, struct Image *image, int x, int y, int frame);
Layer *layer_flatten(Layer *layer, int x, int y, int w, int h, int frmin, int frmax);

#endif /* RASTER_LAYER_H */
