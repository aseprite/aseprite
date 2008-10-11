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

#include <cassert>

#include "jinete/jlist.h"

#include "raster/blend.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "raster/undoable.h"

/**
 * Starts a undoable sequence of operations.
 *
 * All the operations will be grouped in the sprite's undo as an
 * atomic operation.
 */
Undoable::Undoable(Sprite* sprite, const char* label)
{
  assert(sprite);
  assert(label);

  this->sprite = sprite;
  committed = false;
  enabled_flag = undo_is_enabled(sprite->undo);

  if (is_enabled()) {
    undo_set_label(sprite->undo, label);
    undo_open(sprite->undo);
  }
}

Undoable::~Undoable()
{
  if (is_enabled()) {
    // close the undo information
    undo_close(sprite->undo);

    // if it isn't committed, we have to rollback all changes
    if (!committed) {
      // undo the group of operations
      undo_do_undo(sprite->undo);

      // clear the redo (sorry to the user, here we lost the old redo
      // information)
      undo_clear_redo(sprite->undo);
    }
  }
}

/**
 * This must be called to commit all the changes, so the undo will be
 * finally added in the sprite.
 *
 * If you don't use this routine, all the changes will be discarded
 * (if the sprite's undo was enabled when the Undoable was created).
 */
void Undoable::commit()
{
  committed = true;
}

void Undoable::set_number_of_frames(int frames)
{
  assert(frames >= 1);

  // increment frames counter in the sprite
  if (is_enabled())
    undo_set_frames(sprite->undo, sprite);

  sprite_set_frames(sprite, frames);
}

void Undoable::set_current_frame(int frame)
{
  assert(frame >= 0);

  if (is_enabled())
    undo_int(sprite->undo, sprite, &sprite->frame);

  sprite_set_frame(sprite, frame);
}

/**
 * Sets the current selected layer in the sprite.
 *
 * @param layer
 *   The layer to select. Can be NULL.
 */
void Undoable::set_current_layer(Layer* layer)
{
  if (is_enabled())
    undo_set_layer(sprite->undo, sprite);

  sprite_set_layer(sprite, layer);
}

/**
 * Adds a new image in the stock.
 *
 * @return
 *   The image index in the stock.
 */
int Undoable::add_image_in_stock(Image* image)
{
  assert(image);

  // add the image in the stock
  int image_index = stock_add_image(sprite->stock, image);

  if (is_enabled())
    undo_add_image(sprite->undo, sprite->stock, image_index);

  return image_index;
}

/**
 * Removes and destroys the specified image in the stock.
 */
void Undoable::remove_image_from_stock(int image_index)
{
  assert(image_index >= 0);

  Image* image = stock_get_image(sprite->stock, image_index);
  assert(image);

  if (is_enabled())
    undo_remove_image(sprite->undo, sprite->stock, image_index);

  stock_remove_image(sprite->stock, image);
  image_free(image);
}

/**
 * Creates a new transparent layer.
 */
Layer* Undoable::new_layer()
{
  // new layer
  Layer* layer = layer_new(sprite);

  // configure layer name and blend mode
  layer_set_blend_mode(layer, BLEND_MODE_NORMAL);

  // add the layer in the sprite set
  if (is_enabled())
    undo_add_layer(sprite->undo, sprite->set, layer);
  layer_add_layer(sprite->set, layer);

  // select the new layer
  set_current_layer(layer);

  return layer;
}

/**
 * Removes and destroys the specified layer.
 */
void Undoable::remove_layer(Layer* layer)
{
  assert(layer);

  Layer* parent = layer->parent_layer;

  // if the layer to be removed is the selected layer
  if (layer == sprite->layer) {
    Layer* layer_select = NULL;

    // select: previous layer, or next layer, or parent(if it is not the
    // main layer of sprite set)
    if (layer_get_prev(layer))
      layer_select = layer_get_prev(layer);
    else if (layer_get_next(layer))
      layer_select = layer_get_next(layer);
    else if (parent != sprite->set)
      layer_select = parent;

    // select other layer
    set_current_layer(layer_select);
  }

  // remove the layer
  if (is_enabled())
    undo_remove_layer(sprite->undo, layer);

  layer_remove_layer(parent, layer);

  // destroy the layer
  layer_free_images(layer);
  layer_free(layer);
}

void Undoable::move_layer_after(Layer* layer, Layer* after_this)
{
  if (is_enabled())
    undo_move_layer(sprite->undo, layer);

  layer_move_layer(layer->parent_layer, layer, after_this);
}

void Undoable::new_frame()
{
  // add a new cel to every layer
  new_frame_for_layer(sprite->set,
		      sprite->frame+1);

  // increment frames counter in the sprite
  set_number_of_frames(sprite->frames+1);

  // go to next frame (the new one)
  set_current_frame(sprite->frame+1);
}

void Undoable::new_frame_for_layer(Layer* layer, int frame)
{
  assert(layer);
  assert(frame >= 0);

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE:
      // displace all cels in '>=frame' to the next frame
      for (int c=sprite->frames-1; c>=frame; --c) {
	Cel* cel = layer_get_cel(layer, c);
	if (cel)
	  set_cel_frame_position(cel, cel->frame+1);
      }

      copy_previous_frame(layer, frame);
      break;

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	new_frame_for_layer(reinterpret_cast<Layer*>(link->data), frame);
      break;
    }

  }
}

void Undoable::remove_frame(int frame)
{
  assert(frame >= 0);

  // remove cels from this frame (and displace one position backward
  // all next frames)
  remove_frame_of_layer(sprite->set, frame);

  /* decrement frames counter in the sprite */
  if (is_enabled())
    undo_set_frames(sprite->undo, sprite);

  sprite_set_frames(sprite, sprite->frames-1);

  /* move backward if we are outside the range of frames */
  if (sprite->frame >= sprite->frames) {
    if (is_enabled())
      undo_int(sprite->undo, sprite, &sprite->frame);

    sprite_set_frame(sprite, sprite->frames-1);
  }
}

void Undoable::remove_frame_of_layer(Layer* layer, int frame)
{
  assert(layer);
  assert(frame >= 0);

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE:
      if (Cel* cel = layer_get_cel(layer, frame))
	remove_cel(layer, cel);

      for (++frame; frame<sprite->frames; ++frame)
	if (Cel* cel = layer_get_cel(layer, frame))
	  set_cel_frame_position(cel, cel->frame-1);
      break;

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	remove_frame_of_layer(reinterpret_cast<Layer*>(link->data), frame);
      break;
    }

  }
}

/**
 * Copies the previous cel of @a frame to @frame.
 */
void Undoable::copy_previous_frame(Layer* layer, int frame)
{
  assert(layer);
  assert(frame > 0);

  // create a copy of the previous cel
  Cel* src_cel = layer_get_cel(layer, frame-1);
  Image* src_image = src_cel ? stock_get_image(sprite->stock,
					       src_cel->image):
			       NULL;

  // nothing to copy, it will be a transparent cel
  if (!src_image)
    return;

  // copy the image
  Image* dst_image = image_new_copy(src_image);
  int image_index = add_image_in_stock(dst_image);

  // create the new cel
  Cel* dst_cel = cel_new(frame, image_index);
  if (src_cel) {		// copy the data from the previous cel
    cel_set_position(dst_cel, src_cel->x, src_cel->y);
    cel_set_opacity(dst_cel, src_cel->opacity);
  }

  // add the cel in the layer
  add_cel(layer, dst_cel);
}

void Undoable::add_cel(Layer* layer, Cel* cel)
{
  assert(layer);
  assert(cel);
  assert(layer_is_image(layer));

  if (is_enabled())
    undo_add_cel(sprite->undo, layer, cel);

  layer_add_cel(layer, cel);
}

void Undoable::remove_cel(Layer* layer, Cel* cel)
{
  assert(layer);
  assert(cel);
  assert(layer_is_image(layer));

  // find if the image that use the cel to remove, is used by
  // another cels
  bool used = false;
  for (int frame=0; frame<sprite->frames; ++frame) {
    Cel* it = layer_get_cel(layer, frame);
    if (it && it != cel && it->image == cel->image) {
      used = true;
      break;
    }
  }

  // if the image is only used by this cel,
  // we can remove the image from the stock
  if (!used)
    remove_image_from_stock(cel->image);

  if (is_enabled())
    undo_remove_cel(sprite->undo, layer, cel);

  // remove the cel from the layer
  layer_remove_cel(layer, cel);

  // and here we destroy the cel
  cel_free(cel);
}

void Undoable::set_cel_frame_position(Cel* cel, int frame)
{
  assert(cel);
  assert(frame >= 0);

  if (is_enabled())
    undo_int(sprite->undo, cel, &cel->frame);

  cel->frame = frame;
}

void Undoable::set_frame_duration(int frame, int msecs)
{
  if (is_enabled())
    undo_set_frlen(sprite->undo, sprite, frame);

  sprite_set_frlen(sprite, frame, msecs);
}

void Undoable::set_constant_frame_rate(int msecs)
{
  if (is_enabled()) {
    for (int fr=0; fr<sprite->frames; ++fr)
      undo_set_frlen(sprite->undo, sprite, fr);
  }

  sprite_set_speed(sprite, msecs);
}

void Undoable::move_frame_before(int frame, int before_frame)
{
  if (frame != before_frame &&
      frame >= 0 &&
      frame < sprite->frames &&
      before_frame >= 0 &&
      before_frame < sprite->frames) {
    // change the frame-lengths...

    int frlen_aux = sprite->frlens[frame];

    // moving the frame to the future
    if (frame < before_frame) {
      frlen_aux = sprite->frlens[frame];

      for (int c=frame; c<before_frame-1; c++)
	set_frame_duration(c, sprite->frlens[c+1]);

      set_frame_duration(before_frame-1, frlen_aux);
    }
    // moving the frame to the past
    else if (before_frame < frame) {
      frlen_aux = sprite->frlens[frame];

      for (int c=frame; c>before_frame; c--)
	set_frame_duration(c, sprite->frlens[c-1]);

      set_frame_duration(before_frame, frlen_aux);
    }

    // change the cels of position...
    move_frame_before_layer(sprite->set, frame, before_frame);
  }
}

void Undoable::move_frame_before_layer(Layer* layer, int frame, int before_frame)
{
  assert(layer);

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE: {
      JLink link;
      JI_LIST_FOR_EACH(layer->cels, link) {
	Cel* cel = reinterpret_cast<Cel*>(link->data);
	int new_frame = cel->frame;

	// moving the frame to the future
	if (frame < before_frame) {
	  if (cel->frame == frame) {
	    new_frame = before_frame-1;
	  }
	  else if (cel->frame > frame &&
		   cel->frame < before_frame) {
	    new_frame--;
	  }
	}
	// moving the frame to the past
	else if (before_frame < frame) {
	  if (cel->frame == frame) {
	    new_frame = before_frame;
	  }
	  else if (cel->frame >= before_frame &&
		   cel->frame < frame) {
	    new_frame++;
	  }
	}

	if (cel->frame != new_frame)
	  set_cel_frame_position(cel, new_frame);
      }
      break;
    }

    case GFXOBJ_LAYER_SET: {
      JLink link;
      JI_LIST_FOR_EACH(layer->layers, link)
	move_frame_before_layer(reinterpret_cast<Layer*>(link->data), frame, before_frame);
      break;
    }

  }
}

Cel* Undoable::get_current_cel()
{
  if (sprite->layer && layer_is_image(sprite->layer))
    return layer_get_cel(sprite->layer, sprite->frame);
  else
    return NULL;
}

Image* Undoable::get_cel_image(Cel* cel)
{
  if (cel && cel->image >= 0 && cel->image < sprite->stock->nimage)
    return sprite->stock->image[cel->image];
  else
    return NULL;
}

// clears the mask region in the current sprite with the specified background color
void Undoable::clear_mask(int bgcolor)
{
  Cel* cel = get_current_cel();
  if (!cel)
    return;

  Image* image = get_cel_image(cel);
  if (!image)
    return;

  // if the mask is empty then we have to clear the entire image
  // in the cel
  if (mask_is_empty(sprite->mask)) {
    // if the layer is the background then we clear the image
    if (layer_is_background(sprite->layer)) {
      if (is_enabled())
	undo_image(sprite->undo, image, 0, 0, image->w, image->h);

      // clear all
      image_clear(image, bgcolor);
    }
    // if the layer is transparent we can remove the cel (and its
    // associated image)
    else {
      remove_cel(sprite->layer, cel);
    }
  }
  else {
    int u, v, putx, puty;
    int x1 = MAX(0, sprite->mask->x);
    int y1 = MAX(0, sprite->mask->y);
    int x2 = MIN(image->w-1, sprite->mask->x+sprite->mask->w-1);
    int y2 = MIN(image->h-1, sprite->mask->y+sprite->mask->h-1);

    // do nothing
    if (x1 > x2 || y1 > y2)
      return;

    if (is_enabled())
      undo_image(sprite->undo, image, x1, y1, x2-x1+1, y2-y1+1);

    // clear the masked zones
    for (v=0; v<sprite->mask->h; v++) {
      div_t d = div(0, 8);
      ase_uint8* address = ((ase_uint8 **)sprite->mask->bitmap->line)[v]+d.quot;

      for (u=0; u<sprite->mask->w; u++) {
	if ((*address & (1<<d.rem))) {
	  putx = u + sprite->mask->x - cel->x;
	  puty = v + sprite->mask->y - cel->y;
	  image_putpixel(image, putx, puty, bgcolor);
	}

	_image_bitmap_next_bit(d, address);
      }
    }
  }
}

