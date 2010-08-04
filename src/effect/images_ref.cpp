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

#include "jinete/jlist.h"

#include "effect/effect.h"
#include "effect/images_ref.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"

static ImageRef* images_ref_get_from_layer(Sprite* sprite, Layer* layer, int target, bool write);

ImageRef* images_ref_get_from_sprite(Sprite* sprite, int target, bool write)
{
  Layer* layer = target & TARGET_ALL_LAYERS ? sprite->getFolder():
					      sprite->getCurrentLayer();

  return images_ref_get_from_layer(sprite, layer, target, write);
}

void images_ref_free(ImageRef* image_ref)
{
  ImageRef *p, *next;

  for (p=image_ref; p; p=next) {
    next = p->next;
    jfree(p);
  }
}

static ImageRef* images_ref_get_from_layer(Sprite* sprite, Layer* layer, int target, bool write)
{
#define ADD_IMAGES(images)			\
  {						\
    if (first_image == NULL) {			\
      first_image = images;			\
      last_image = images;			\
    }						\
    else {					\
      ASSERT(last_image != NULL);		\
      last_image->next = images;		\
    }						\
						\
    while (last_image->next != NULL)		\
      last_image = last_image->next;		\
  }

#define NEW_IMAGE(layer, cel)					\
  {								\
    ImageRef* image_ref = jnew(ImageRef, 1);			\
								\
    image_ref->image = layer->getSprite()->getStock()->image[cel->image]; \
    image_ref->layer = layer;					\
    image_ref->cel = cel;					\
    image_ref->next = NULL;					\
								\
    ADD_IMAGES(image_ref);					\
  }
  
  ImageRef* first_image = NULL;
  ImageRef* last_image = NULL;
  int frame = sprite->getCurrentFrame();

  if (!layer->is_readable())
    return NULL;

  if (write && !layer->is_writable())
    return NULL;

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE: {
      if (target & TARGET_ALL_FRAMES) {
	for (frame=0; frame<sprite->getTotalFrames(); frame++) {
	  Cel* cel = static_cast<LayerImage*>(layer)->get_cel(frame);
	  if (cel != NULL)
	    NEW_IMAGE(layer, cel);
	}
      }
      else {
	Cel* cel = static_cast<LayerImage*>(layer)->get_cel(frame);
	if (cel != NULL)
	  NEW_IMAGE(layer, cel);
      }
      break;
    }

    case GFXOBJ_LAYER_FOLDER: {
      ImageRef* sub_images;
      LayerIterator it = static_cast<LayerFolder*>(layer)->get_layer_begin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it) {
	sub_images = images_ref_get_from_layer(sprite, *it, target, write);
	if (sub_images != NULL)
	  ADD_IMAGES(sub_images);
      }
      break;
    }

  }

  return first_image;
}
