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

#include "config.h"

#include <assert.h>
#include "jinete/jinete.h"

#include "ase/current_sprite.h"
#include "ase/ui_context.h"
#include "console/console.h"
#include "core/app.h"
#include "file/file.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/raster.h"
#include "util/functions.h"
#include "util/misc.h"
#include "widgets/colbar.h"

/*===================================================================*/
/* Sprite                                                            */
/*===================================================================*/

/**
 * Creates a new sprite with the given dimension with one transparent
 * layer called "Layer 1".
 *
 * @param imgtype Color mode, one of the following values: IMAGE_RGB, IMAGE_GRAYSCALE, IMAGE_INDEXED
 * @param w Width of the sprite
 * @param h Height of the sprite
 */
Sprite *NewSprite(int imgtype, int w, int h)
{
  Sprite *sprite;

  if (imgtype != IMAGE_RGB &&
      imgtype != IMAGE_GRAYSCALE &&
      imgtype != IMAGE_INDEXED) {
    console_printf("NewSprite: argument 'imgtype' should be IMAGE_RGB, IMAGE_GRAYSCALE or IMAGE_INDEXED\n");
    return NULL;
  }

  if (w < 1 || w > 9999) {
    console_printf("NewSprite: argument 'w' should be between 1 and 9999\n");
    return NULL;
  }

  if (h < 1 || h > 9999) {
    console_printf("NewSprite: argument 'h' should be between 1 and 9999\n");
    return NULL;
  }

  sprite = sprite_new_with_layer(imgtype, w, h);
  if (!sprite)
    return NULL;

  undo_disable(sprite->undo);

  UIContext* context = UIContext::instance();
  context->add_sprite(sprite);
  context->set_current_sprite(sprite);

  assert(undo_is_disabled(sprite->undo));
  return sprite;
}

/**
 * Loads a sprite from the specified file.
 *
 * If the sprite is succefully loaded it'll be selected as the current
 * sprite (@ref SetSprite) automatically.
 *
 * @param filename The name of the file. Can be absolute or relative
 *                 to the location which ASE was executed.
 *
 * @return The loaded sprite or nil if it couldn't be read.
 */
Sprite *LoadSprite(const char *filename)
{
  Sprite *sprite;

  if (filename == NULL) {
    console_printf("LoadSprite: filename can't be NULL\n");
    return NULL;
  }

  sprite = sprite_load(filename);
  if (sprite) {
    undo_disable(sprite->undo);

    UIContext* context = UIContext::instance();
    context->add_sprite(sprite);
    context->set_current_sprite(sprite);
  }

  assert(undo_is_disabled(sprite->undo));
  return sprite;
}

/**
 * Saves the current sprite.
 */
void SaveSprite(const char *filename)
{
  CurrentSprite sprite;

  if (sprite == NULL) {
    console_printf("SaveSprite: No current sprite\n");
    return;
  }

  if (filename == NULL) {
    console_printf("SaveSprite: filename can't be NULL\n");
    return;
  }

  sprite_set_filename(sprite, filename);
  app_realloc_sprite_list();

  if (sprite_save(sprite) == 0)
    sprite_mark_as_saved(sprite);
  else
    console_printf("SaveSprite: Error saving sprite file %s\n", filename);
}

/**
 * Sets the specified sprite as the current sprite to apply all next
 * operations.
 */
void SetSprite(Sprite *sprite)
{
  UIContext* context = UIContext::instance();
  context->set_current_sprite(sprite);
}

/*===================================================================*/
/* Layer                                                             */
/*===================================================================*/

static int get_max_layer_num(Layer *layer);

/**
 * Creates a new layer set with the "name" in the current sprite
 */
Layer *NewLayerSet(Sprite *sprite)
{
  Layer *layer = NULL;

  if (sprite == NULL) {
    console_printf("NewLayer: No current sprite\n");
    return NULL;
  }

  /* new layer */
  layer = layer_set_new(sprite);
  if (!layer)
    return NULL;

  /* configure layer name and blend mode */
  /* TODO */
  /*     { */
  /*       char *name; */
  /*       name = GetUniqueLayerName(); */
  /*       layer_set_name(layer, name); */
  /*       jfree(name); */
  /*     } */

  /* add the layer in the sprite set */
  layer_add_layer(sprite->set, layer);

  /* select the new layer */
  sprite_set_layer(sprite, layer);

  return layer;
}

char *GetUniqueLayerName(Sprite *sprite)
{
  if (sprite != NULL) {
    char buf[1024];
    sprintf(buf, "Layer %d", get_max_layer_num(sprite->set)+1);
    return jstrdup(buf);
  }
  else
    return NULL;
}

Layer *FlattenLayers(Sprite *sprite)
{
  JLink link, next;
  Layer *background;
  Image *image;
  Image *cel_image;
  Cel *cel;
  int frame;
  int bgcolor;

  /* there are a current sprite selected? */
  if (!sprite)
    return NULL;

  /* create a temporary image */
  image = image_new(sprite->imgtype, sprite->w, sprite->h);
  if (!image) {
    console_printf("Not enough memory");
    return NULL;
  }

  /* open undo */
  if (undo_is_enabled(sprite->undo))
    undo_open(sprite->undo);

  /* get the background layer from the sprite */
  background = sprite_get_background_layer(sprite);
  if (!background) {
    /* if there aren't a background layer we must to create the background */
    background = layer_new(sprite);
    if (!background) {
      image_free(image);
      console_printf("Not enough memory");
      return NULL;
    }

    if (undo_is_enabled(sprite->undo))
      undo_add_layer(sprite->undo, sprite->set, background);

    layer_add_layer(sprite->set, background);

    if (undo_is_enabled(sprite->undo))
      undo_move_layer(sprite->undo, background);
    
    layer_configure_as_background(background);

    /* get the color to clean the temporary image in each frame */
    bgcolor = get_color_for_image(sprite->imgtype,
				  colorbar_get_bg_color(app_get_colorbar()));
  }
  else
    bgcolor = 0;

  /* copy all frames to the background */
  for (frame=0; frame<sprite->frames; frame++) {
    /* clear the image and render this frame */
    image_clear(image, bgcolor);
    layer_render(sprite->set, image, 0, 0, frame);

    cel = layer_get_cel(background, frame);
    if (cel) {
      cel_image = sprite->stock->image[cel->image];
      assert(cel_image != NULL);

      /* we have to save the current state of `cel_image' in the undo */
      if (undo_is_enabled(sprite->undo)) {
	Dirty *dirty = dirty_new_from_differences(cel_image, image);
	dirty_save_image_data(dirty);
	/* TODO error handling: if (dirty == NULL) */
	if (dirty != NULL)
	  undo_dirty(sprite->undo, dirty);
      }
    }
    else {
      /* if there aren't a cel in this frame in the background, we
	 have to create a copy of the image for the new cel */
      cel_image = image_new_copy(image);
      /* TODO error handling: if (!cel_image) { ... } */

      /* here we create the new cel (with the new image `cel_image') */
      cel = cel_new(frame, stock_add_image(sprite->stock, cel_image));
      /* TODO error handling: if (!cel) { ... } */

      /* and finally we add the cel in the background */
      layer_add_cel(background, cel);
    }

    image_copy(cel_image, image, 0, 0);
  }

  /* select the background */
  if (sprite->layer != background) {
    if (undo_is_enabled(sprite->undo))
      undo_set_layer(sprite->undo, sprite);

    sprite_set_layer(sprite, background);
  }

  /* remove old layers */
  JI_LIST_FOR_EACH_SAFE(sprite->set->layers, link, next) {
    if (link->data != background) {
      Layer* old_layer = reinterpret_cast<Layer*>(link->data);

      /* remove the layer */
      if (undo_is_enabled(sprite->undo))
	undo_remove_layer(sprite->undo, old_layer);

      layer_remove_layer(sprite->set, old_layer);

      /* destroy the layer */
      layer_free_images(old_layer);
      layer_free(old_layer);
    }
  }

  /* destroy the temporary image */
  image_free(image);

  /* close the undo */
  if (undo_is_enabled(sprite->undo))
    undo_close(sprite->undo);
 
  return background;
}

void LayerFromBackground(Sprite *sprite)
{
  if (sprite == NULL) {
    console_printf("LayerFromBackground: there is not a current sprite selected\n");
    return;
  }

  if (sprite_get_background_layer(sprite) == NULL) {
    console_printf("LayerFromBackground: the current sprite hasn't a `Background' layer.\n");
    return;
  }

  if (sprite->layer == NULL) {
    console_printf("LayerFromBackground: there is not a current layer selected.\n");
    return;
  }

  if (!layer_is_image(sprite->layer)) {
    console_printf("LayerFromBackground: the current layer must be an `image' layer.\n");
    return;
  }

  if (!layer_is_readable(sprite->layer)) {
    console_printf("LayerFromBackground: the current layer is hidden, can't be converted.\n");
    return;
  }

  if (!layer_is_writable(sprite->layer)) {
    console_printf("LayerFromBackground: the current layer is locked, can't be converted.\n");
    return;
  }

  if (!layer_is_background(sprite->layer)) {
    console_printf("LayerFromBackground: the current layer must be the `Background' layer.\n");
    return;
  }

  if (undo_is_enabled(sprite->undo)) {
    undo_open(sprite->undo);
    undo_data(sprite->undo,
	      (GfxObj *)sprite->layer,
	      &sprite->layer->flags,
	      sizeof(sprite->layer->flags));
    undo_data(sprite->undo,
	      (GfxObj *)sprite->layer,
	      &sprite->layer->name,
	      LAYER_NAME_SIZE);
    undo_close(sprite->undo);
  }

  sprite->layer->flags &= ~(LAYER_IS_LOCKMOVE | LAYER_IS_BACKGROUND);
  layer_set_name(sprite->layer, "Layer 0");
}

/* internal routine */
static int get_max_layer_num(Layer *layer)
{
  int max = 0;

  if (strncmp(layer->name, "Layer ", 6) == 0)
    max = strtol(layer->name+6, NULL, 10);

  if (layer_is_set(layer)) {
    JLink link;
    JI_LIST_FOR_EACH(layer->layers, link) {
      int tmp = get_max_layer_num(reinterpret_cast<Layer*>(link->data));
      max = MAX(tmp, max);
    }
  }
  
  return max;
}

/* ======================================= */
/* Cel                                     */
/* ======================================= */

void RemoveCel(Layer *layer, Cel *cel)
{
  CurrentSprite sprite;
  Image *image;
  Cel *it;
  int frame;
  bool used;

  if (sprite != NULL && layer_is_image(layer) && cel != NULL) {
    /* find if the image that use the cel to remove, is used by
       another cels */
    used = FALSE;
    for (frame=0; frame<sprite->frames; ++frame) {
      it = layer_get_cel(layer, frame);
      if (it != NULL && it != cel && it->image == cel->image) {
	used = TRUE;
	break;
      }
    }

    if (undo_is_enabled(sprite->undo))
      undo_open(sprite->undo);

    if (!used) {
      /* if the image is only used by this cel, we can remove the
	 image from the stock */
      image = stock_get_image(sprite->stock, cel->image);

      if (undo_is_enabled(sprite->undo))
	undo_remove_image(sprite->undo, sprite->stock, cel->image);

      stock_remove_image(sprite->stock, image);
      image_free(image);
    }

    if (undo_is_enabled(sprite->undo)) {
      undo_remove_cel(sprite->undo, layer, cel);
      undo_close(sprite->undo);
    }

    /* remove the cel */
    layer_remove_cel(layer, cel);
    cel_free(cel);
  }
}
