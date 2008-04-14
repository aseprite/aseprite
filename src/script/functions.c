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

#include <assert.h>
#include "jinete/jinete.h"

#include "console/console.h"
#include "core/app.h"
#include "file/file.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/raster.h"
#include "util/misc.h"
#include "widgets/colbar.h"

/*===================================================================*/
/* Sprite                                                            */
/*===================================================================*/

static void displace_layers(Undo *undo, Layer *layer, int x, int y);

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
  sprite_mount(sprite);
  set_current_sprite(sprite);

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
    sprite_mount(sprite);
    set_current_sprite(sprite);
  }

  assert(undo_is_disabled(sprite->undo));
  return sprite;
}

/**
 * Saves the current sprite.
 */
void SaveSprite(const char *filename)
{
  if (current_sprite == NULL) {
    console_printf("SaveSprite: No current sprite\n");
    return;
  }

  if (filename == NULL) {
    console_printf("SaveSprite: filename can't be NULL\n");
    return;
  }

  sprite_set_filename(current_sprite, filename);
  app_realloc_sprite_list();

  if (sprite_save(current_sprite) == 0)
    sprite_mark_as_saved(current_sprite);
  else
    console_printf("SaveSprite: Error saving sprite file %s\n", filename);
}

/**
 * Sets the specified sprite as the current sprite to apply all next
 * operations.
 */
void SetSprite(Sprite *sprite)
{
  set_current_sprite(sprite);
}

void CropSprite(void)
{
  Sprite *sprite = current_sprite;

  if ((sprite) &&
      (!mask_is_empty(sprite->mask))) {
    if (undo_is_enabled(sprite->undo)) {
      undo_set_label(sprite->undo, "Sprite Crop");
      undo_open(sprite->undo);
      undo_int(sprite->undo, (GfxObj *)sprite, &sprite->w);
      undo_int(sprite->undo, (GfxObj *)sprite, &sprite->h);
    }

    sprite_set_size(sprite, sprite->mask->w, sprite->mask->h);

    displace_layers(sprite->undo, sprite->set,
		    -sprite->mask->x, -sprite->mask->y);

    if (undo_is_enabled(sprite->undo)) {
      undo_int(sprite->undo, (GfxObj *)sprite->mask, &sprite->mask->x);
      undo_int(sprite->undo, (GfxObj *)sprite->mask, &sprite->mask->y);
    }

    sprite->mask->x = 0;
    sprite->mask->y = 0;

    if (undo_is_enabled(sprite->undo))
      undo_close(sprite->undo);

    sprite_generate_mask_boundaries(sprite);
    update_screen_for_sprite(sprite);
  }
}

/**
 * Moves every frame in "layer" with the offset "x"/"y".
 */
static void displace_layers(Undo *undo, Layer *layer, int x, int y)
{
  switch (layer->gfxobj.type) {

    case GFXOBJ_LAYER_IMAGE: {
      Cel *cel;
      JLink link;

      JI_LIST_FOR_EACH(layer->cels, link) {
	cel = link->data;

	if (undo_is_enabled(undo)) {
	  undo_int(undo, (GfxObj *)cel, &cel->x);
	  undo_int(undo, (GfxObj *)cel, &cel->y);
	}

	cel->x += x;
	cel->y += y;
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	displace_layers(undo, link->data, x, y);
      break;
    }

  }
}

/*===================================================================*/
/* Layer                                                             */
/*===================================================================*/

static int get_max_layer_num(Layer *layer);

/**
 * Creates a new layer with one cel in the current frame of the
 * sprite.
 *
 * @param name Name of the layer (could be NULL)
 * @param x Horizontal position of the first cel
 * @param y Vertical position of the first cel
 * @param w
 * @param h
 *
 * with the specified position and size (if w=h=0 the
 * routine will use the sprite dimension)
 */
Layer *NewLayer(void)
{
  Sprite *sprite = current_sprite;
  Layer *layer;
  Image *image;
  Cel *cel;
  int index;

  if (sprite == NULL) {
    console_printf("NewLayer: No current sprite\n");
    return NULL;
  }

  /* new image */
  image = image_new(sprite->imgtype, sprite->w, sprite->h);
  if (!image) {
    console_printf("NewLayer: Not enough memory\n");
    return NULL;
  }

  /* new layer */
  layer = layer_new(sprite);
  if (!layer) {
    image_free(image);
    console_printf("NewLayer: Not enough memory\n");
    return NULL;
  }

  /* clear with mask color */
  image_clear(image, 0);

  /* configure layer name and blend mode */
    /* TODO */
/*     { */
/*       char *name; */
/*       name = GetUniqueLayerName(); */
/*       layer_set_name(layer, name); */
/*       jfree(name); */
/*     } */
  layer_set_blend_mode(layer, BLEND_MODE_NORMAL);

  /* add image in the layer stock */
  index = stock_add_image(sprite->stock, image);

  /* create a new cel in the current frame */
  cel = cel_new(sprite->frame, index);

  /* add cel */
  layer_add_cel(layer, cel);

  /* undo stuff */
  if (undo_is_enabled(sprite->undo)) {
    undo_set_label(sprite->undo, "New Layer");
    undo_open(sprite->undo);
    undo_add_layer(sprite->undo, sprite->set, layer);
    undo_set_layer(sprite->undo, sprite);
    undo_close(sprite->undo);
  }

  /* add the layer in the sprite set */
  layer_add_layer(sprite->set, layer);

  /* select the new layer */
  sprite_set_layer(sprite, layer);

  return layer;
}

/**
 * Creates a new layer set with the "name" in the current sprite
 */
Layer *NewLayerSet(void)
{
  Sprite *sprite = current_sprite;
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

/**
 * Removes the current selected layer
 */
void RemoveLayer(void)
{
  Sprite *sprite = current_sprite;
  if (sprite && sprite->layer) {
    Layer *layer = sprite->layer;
    Layer *parent = layer->parent_layer;
    Layer *layer_select;

    /* select: previous layer, or next layer, or parent(if it is not
       the main layer of sprite set) */
    if (layer_get_prev(layer))
      layer_select = layer_get_prev(layer);
    else if (layer_get_next(layer))
      layer_select = layer_get_next(layer);
    else if (parent != sprite->set)
      layer_select = parent;
    else
      layer_select = NULL;

    /* undo stuff */
    if (undo_is_enabled(sprite->undo)) {
      undo_set_label(sprite->undo, "Remove Layer");
      undo_open(sprite->undo);
    }

    /* select other layer */
    if (undo_is_enabled(sprite->undo))
      undo_set_layer(sprite->undo, sprite);
    sprite_set_layer(sprite, layer_select);

    /* remove the layer */
    if (undo_is_enabled(sprite->undo))
      undo_remove_layer(sprite->undo, layer);
    layer_remove_layer(parent, layer);

    /* destroy the layer */
    layer_free_images(layer);
    layer_free(layer);

    /* close undo */
    if (undo_is_enabled(sprite->undo))
      undo_close(sprite->undo);
  }
}

char *GetUniqueLayerName(void)
{
  Sprite *sprite = current_sprite;
  if (sprite) {
    char buf[1024];
    sprintf(buf, "Layer %d", get_max_layer_num(sprite->set)+1);
    return jstrdup(buf);
  }
  else
    return NULL;
}

Layer *FlattenLayers(void)
{
  Sprite *sprite = current_sprite;
  bool is_new_background = FALSE;
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

    layer_configure_as_background(background);

    is_new_background = TRUE;

    /* get the color to clean the temporary image in each frame */
    bgcolor = get_color_for_image(sprite->imgtype,
				  colorbar_get_bg_color(app_get_colorbar()));
  }
  else
    bgcolor = 0;

  /* open undo */
  if (undo_is_enabled(sprite->undo)) {
    undo_set_label(sprite->undo, "Flatten Layers");
    undo_open(sprite->undo);
  }
  
  /* add the new layer */
  if (is_new_background) {
    if (undo_is_enabled(sprite->undo))
      undo_add_layer(sprite->undo, sprite->set, background);

    layer_add_layer(sprite->set, background);
  }

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
	 have to create a copy of the image for the new cel which will
	 be created */
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
      Layer *old_layer = link->data;

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

#if 0				/* TODO why? */
  /* update all editors that has this sprite */
  update_screen_for_sprite(sprite);
#endif
 
  return background;
}

void CropLayer(void)
{
  Sprite *sprite = current_sprite;

  if ((sprite != NULL) &&
      (!mask_is_empty(sprite->mask)) &&
      (sprite->layer != NULL) &&
      (layer_is_image(sprite->layer))) {
    Layer *layer = sprite->layer;
    Cel *cel;
    Image *image;
    Layer *new_layer;
    Cel *new_cel;
    Image *new_image;
    Layer *set = layer->parent_layer;
    JLink link;

    new_layer = layer_new(sprite);
    if (!new_layer) {
      console_printf(_("Not enough memory\n"));
      return;
    }

    layer_set_name(new_layer, layer->name);
    layer_set_blend_mode(new_layer, layer->blend_mode);

    JI_LIST_FOR_EACH(layer->cels, link) {
      cel = link->data;
      image = stock_get_image(sprite->stock, cel->image);
      if (!image)
	continue;

      new_cel = cel_new_copy(cel);
      if (!new_cel) {
	layer_free(new_layer);
	console_printf(_("Not enough memory\n"));
	return;
      }

      new_image = image_crop(image,
			     sprite->mask->x-cel->x,
			     sprite->mask->y-cel->y,
			     sprite->mask->w,
			     sprite->mask->h);
      if (!new_image) {
	layer_free(new_layer);
	cel_free(new_cel);
	console_printf(_("Not enough memory\n"));
	return;
      }

      new_cel->image = stock_add_image(sprite->stock, new_image);
      new_cel->x = sprite->mask->x;
      new_cel->y = sprite->mask->y;

      layer_add_cel(new_layer, new_cel);
    }

    /* add the new layer */
    if (undo_is_enabled(sprite->undo)) {
      undo_set_label(sprite->undo, "Layer Crop");
      undo_open(sprite->undo);
      undo_add_layer(sprite->undo, set, new_layer);
    }

    layer_add_layer(set, new_layer);

    /* move it after the old one */
    if (undo_is_enabled(sprite->undo))
      undo_move_layer(sprite->undo, new_layer);

    layer_move_layer(set, new_layer, layer);

    /* set the new one as the current one */
    if (undo_is_enabled(sprite->undo))
      undo_set_layer(sprite->undo, sprite);

    sprite_set_layer(sprite, new_layer);

    /* remove the old layer */
    if (undo_is_enabled(sprite->undo)) {
      undo_remove_layer(sprite->undo, layer);
      undo_close(sprite->undo);
    }

    layer_remove_layer(set, layer);

    layer_free_images(layer);
    layer_free(layer);

    /* refresh */
    update_screen_for_sprite(sprite);
  }
}

/**
 * Converts the selected layer in a `Background' layer.
 */
void BackgroundFromLayer(void)
{
  Sprite *sprite;
  int bgcolor;
  JLink link;
  Image *bg_image;
  Image *cel_image;

  sprite = current_sprite;
  if (sprite == NULL) {
    console_printf("BackgroundFromLayer: there is not a current sprite selected\n");
    return;
  }

  if (sprite_get_background_layer(sprite) != NULL) {
    console_printf("BackgroundFromLayer: the current sprite already has a `Background' layer.\n");
    return;
  }

  if (sprite->layer == NULL) {
    console_printf("BackgroundFromLayer: there are not a current layer selected.\n");
    return;
  }

  if (!layer_is_image(sprite->layer)) {
    console_printf("BackgroundFromLayer: the current layer must be an `image' layer.\n");
    return;
  }

  if (!layer_is_readable(sprite->layer)) {
    console_printf("BackgroundFromLayer: the current layer is hidden, can't be converted.\n");
    return;
  }

  if (!layer_is_writable(sprite->layer)) {
    console_printf("BackgroundFromLayer: the current layer is locked, can't be converted.\n");
    return;
  }

  /* each frame of the layer to be converted as `Background' must be
     cleared using the selected background color in the color-bar */
  bgcolor = app_get_bg_color(sprite);

  if (undo_is_enabled(sprite->undo)) {
    undo_set_label(sprite->undo, "Background from Layer");
    undo_open(sprite->undo);
  }

  /* create a temporary image to draw each frame of the new
     `Background' layer */
  bg_image = image_new(sprite->imgtype, sprite->w, sprite->h);

  JI_LIST_FOR_EACH(sprite->layer->cels, link) {
    Cel *cel = link->data;
    assert((cel->image >= 0) &&
	   (cel->image < sprite->stock->nimage));

    /* get the image from the sprite's stock of images */
    cel_image = sprite->stock->image[cel->image];
    assert(cel_image != NULL);

    image_clear(bg_image, bgcolor);
    image_merge(bg_image, cel_image,
		cel->x,
		cel->y,
		MID(0, cel->opacity, 255),
		sprite->layer->blend_mode);

    /* now we have to copy the new image (bg_image) to the cel... */

    if (undo_is_enabled(sprite->undo)) {
      if (cel->x != 0) undo_int(sprite->undo, (GfxObj *)cel, &cel->x);
      if (cel->y != 0) undo_int(sprite->undo, (GfxObj *)cel, &cel->y);
    }

    /* same size of cel-image and bg-image */
    if (bg_image->w == cel_image->w &&
	bg_image->h == cel_image->h) {
      if (undo_is_enabled(sprite->undo))
	undo_image(sprite->undo, cel_image, 0, 0, cel_image->w, cel_image->h);

      image_copy(cel_image, bg_image, 0, 0);
    }
    else {
      if (undo_is_enabled(sprite->undo))
	undo_replace_image(sprite->undo, sprite->stock, cel->image);

      /* replace the image */
      sprite->stock->image[cel->image] = image_new_copy(bg_image);

      image_free(cel_image);
    }

    /* change the cel position */
    cel->x = 0;
    cel->y = 0;
  }

  image_free(bg_image);

  /* new configuration for the `Background' layer */
  if (undo_is_enabled(sprite->undo)) {
    undo_data(sprite->undo,
	      (GfxObj *)sprite->layer,
	      &sprite->layer->flags,
	      sizeof(sprite->layer->flags));
    undo_data(sprite->undo,
	      (GfxObj *)sprite->layer,
	      &sprite->layer->name,
	      LAYER_NAME_SIZE);
    undo_move_layer(sprite->undo, sprite->layer);
  }

  layer_configure_as_background(sprite->layer);

  if (undo_is_enabled(sprite->undo))
    undo_close(sprite->undo);
}

void LayerFromBackground(void)
{
  Sprite *sprite;

  sprite = current_sprite;
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
    undo_set_label(sprite->undo, "Layer from Background");
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
      int tmp = get_max_layer_num(link->data);
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
  Sprite *sprite = current_sprite;
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

    if (undo_is_enabled(sprite->undo)) {
      undo_set_label(sprite->undo, "Remove Cel");
      undo_open(sprite->undo);
    }

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

void CropCel(void)
{
  Sprite *sprite = current_sprite;
  Image *image = GetImage(current_sprite);

  if ((sprite) && (!mask_is_empty (sprite->mask)) && (image)) {
    Cel *cel = layer_get_cel(sprite->layer, sprite->frame);

    /* undo */
    if (undo_is_enabled(sprite->undo)) {
      undo_set_label(sprite->undo, "Cel Crop");
      undo_open(sprite->undo);
      undo_int(sprite->undo, (GfxObj *)cel, &cel->x);
      undo_int(sprite->undo, (GfxObj *)cel, &cel->y);
      undo_replace_image(sprite->undo, sprite->stock, cel->image);
      undo_close(sprite->undo);
    }

    /* replace the image */
    sprite->stock->image[cel->image] =
      image_crop(image,
		 sprite->mask->x-cel->x,
		 sprite->mask->y-cel->y,
		 sprite->mask->w,
		 sprite->mask->h);

    image_free(image);		/* destroy the old image */

    /* change the cel position */
    cel->x = sprite->mask->x;
    cel->y = sprite->mask->y;

    update_screen_for_sprite(sprite);
  }
}
