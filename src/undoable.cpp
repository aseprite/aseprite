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

#include <cassert>
#include <memory>

#include "jinete/jlist.h"

#include "sprite_wrappers.h"
#include "raster/blend.h"
#include "raster/cel.h"
#include "raster/dirty.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/palette.h"
#include "raster/quant.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "undoable.h"

/**
 * Starts a undoable sequence of operations.
 *
 * All the operations will be grouped in the sprite's undo as an
 * atomic operation.
 */
Undoable::Undoable(SpriteWriter& sprite, const char* label)
{
  assert(label != NULL);

  m_sprite = sprite;
  m_committed = false;
  m_enabled_flag = undo_is_enabled(m_sprite->undo);

  if (is_enabled()) {
    undo_set_label(m_sprite->undo, label);
    undo_open(m_sprite->undo);
  }
}

Undoable::~Undoable()
{
  if (is_enabled()) {
    // close the undo information
    undo_close(m_sprite->undo);

    // if it isn't committed, we have to rollback all changes
    if (!m_committed) {
      // undo the group of operations
      undo_do_undo(m_sprite->undo);

      // clear the redo (sorry to the user, here we lost the old redo
      // information)
      undo_clear_redo(m_sprite->undo);
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
  m_committed = true;
}

void Undoable::set_number_of_frames(int frames)
{
  assert(frames >= 1);

  // increment frames counter in the sprite
  if (is_enabled())
    undo_set_frames(m_sprite->undo, m_sprite);

  sprite_set_frames(m_sprite, frames);
}

void Undoable::set_current_frame(int frame)
{
  assert(frame >= 0);

  if (is_enabled())
    undo_int(m_sprite->undo, m_sprite, &m_sprite->frame);

  sprite_set_frame(m_sprite, frame);
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
    undo_set_layer(m_sprite->undo, m_sprite);

  sprite_set_layer(m_sprite, layer);
}

void Undoable::set_sprite_size(int w, int h)
{
  assert(w > 0);
  assert(h > 0);

  if (is_enabled()) {
    undo_int(m_sprite->undo, m_sprite, &m_sprite->w);
    undo_int(m_sprite->undo, m_sprite, &m_sprite->h);
  }

  sprite_set_size(m_sprite, w, h);
}

void Undoable::crop_sprite(int x, int y, int w, int h, int bgcolor)
{
  set_sprite_size(w, h);

  displace_layers(m_sprite->get_folder(), -x, -y);

  Layer *background_layer = sprite_get_background_layer(m_sprite);
  if (background_layer)
    crop_layer(background_layer, 0, 0, m_sprite->w, m_sprite->h, bgcolor);

  if (!m_sprite->mask->is_empty())
    set_mask_position(m_sprite->mask->x-x, m_sprite->mask->y-y);
}

void Undoable::autocrop_sprite(int bgcolor)
{
  int old_frame = m_sprite->frame;
  int x1, y1, x2, y2;
  int u1, v1, u2, v2;

  x1 = y1 = INT_MAX;
  x2 = y2 = INT_MIN;

  Image* image = image_new(m_sprite->imgtype, m_sprite->w, m_sprite->h);

  for (m_sprite->frame=0; m_sprite->frame<m_sprite->frames; m_sprite->frame++) {
    image_clear(image, 0);
    sprite_render(m_sprite, image, 0, 0);

    // TODO configurable (what color pixel to use as "refpixel",
    // here we are using the top-left pixel by default)
    if (image_shrink_rect(image, &u1, &v1, &u2, &v2,
			  image_getpixel(image, 0, 0))) {
      x1 = MIN(x1, u1);
      y1 = MIN(y1, v1);
      x2 = MAX(x2, u2);
      y2 = MAX(y2, v2);
    }
  }
  m_sprite->frame = old_frame;

  image_free(image);

  // do nothing
  if (x1 > x2 || y1 > y2)
    return;

  crop_sprite(x1, y1, x2-x1+1, y2-y1+1, bgcolor);
}

/**
 * @warning: it uses the current Allegro "rgb_map"
 */
void Undoable::set_imgtype(int new_imgtype, int dithering_method)
{
  Image *old_image;
  Image *new_image;
  int c;

  if (m_sprite->imgtype == new_imgtype)
    return;

  /* change imgtype of the stock of images */
  if (is_enabled())
    undo_int(m_sprite->undo, (GfxObj *)m_sprite->stock, &m_sprite->stock->imgtype);

  m_sprite->stock->imgtype = new_imgtype;

  for (c=0; c<m_sprite->stock->nimage; c++) {
    old_image = stock_get_image(m_sprite->stock, c);
    if (!old_image)
      continue;

    new_image = image_set_imgtype(old_image, new_imgtype, dithering_method,
				  rgb_map,
				  /* TODO check this out */
				  sprite_get_palette(m_sprite,
						     m_sprite->frame));
    if (!new_image)
      return;		/* TODO error handling: not enough memory!
			   we should undo all work done */

    if (is_enabled())
      undo_replace_image(m_sprite->undo, m_sprite->stock, c);

    image_free(old_image);
    stock_replace_image(m_sprite->stock, c, new_image);
  }

  /* change "sprite.imgtype" field */
  if (is_enabled())
    undo_int(m_sprite->undo, m_sprite, &m_sprite->imgtype);

  m_sprite->imgtype = new_imgtype;

  // change "sprite.palette"
  if (new_imgtype == IMAGE_GRAYSCALE) {
    if (is_enabled()) {
      Palette* palette;
      JLink link;

      JI_LIST_FOR_EACH(m_sprite->palettes, link) {
	if (jlist_first(m_sprite->palettes) != link) {
	  palette = reinterpret_cast<Palette*>(link->data);
	  undo_remove_palette(m_sprite->undo, m_sprite, palette);
	}
      }

      palette = sprite_get_palette(m_sprite, 0);
      undo_data(m_sprite->undo, palette, palette->color, sizeof(palette->color));
    }

    Palette* graypal = palette_new(0, MAX_PALETTE_COLORS);
    for (int c=0; c<256; c++)
      palette_set_entry(graypal, c, _rgba(c, c, c, 255));

    sprite_reset_palettes(m_sprite);
    sprite_set_palette(m_sprite, graypal, false);

    palette_free(graypal);
  }
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
  int image_index = stock_add_image(m_sprite->stock, image);

  if (is_enabled())
    undo_add_image(m_sprite->undo, m_sprite->stock, image_index);

  return image_index;
}

/**
 * Removes and destroys the specified image in the stock.
 */
void Undoable::remove_image_from_stock(int image_index)
{
  assert(image_index >= 0);

  Image* image = stock_get_image(m_sprite->stock, image_index);
  assert(image);

  if (is_enabled())
    undo_remove_image(m_sprite->undo, m_sprite->stock, image_index);

  stock_remove_image(m_sprite->stock, image);
  image_free(image);
}

void Undoable::replace_stock_image(int image_index, Image* new_image)
{
  // get the current image in the 'image_index' position
  Image* old_image = stock_get_image(m_sprite->stock, image_index);
  assert(old_image);

  // replace the image in the stock
  if (is_enabled())
    undo_replace_image(m_sprite->undo, m_sprite->stock, image_index);

  stock_replace_image(m_sprite->stock, image_index, new_image);

  // destroy the old image
  image_free(old_image);
}

/**
 * Creates a new transparent layer.
 */
Layer* Undoable::new_layer()
{
  // new layer
  LayerImage* layer = new LayerImage(m_sprite);

  // configure layer name and blend mode
  layer->set_blend_mode(BLEND_MODE_NORMAL);

  // add the layer in the sprite set
  if (is_enabled())
    undo_add_layer(m_sprite->undo, m_sprite->get_folder(), layer);

  m_sprite->get_folder()->add_layer(layer);

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

  LayerFolder* parent = layer->get_parent();

  // if the layer to be removed is the selected layer
  if (layer == m_sprite->layer) {
    Layer* layer_select = NULL;

    // select: previous layer, or next layer, or parent(if it is not the
    // main layer of sprite set)
    if (layer->get_prev())
      layer_select = layer->get_prev();
    else if (layer->get_next())
      layer_select = layer->get_next();
    else if (parent != m_sprite->get_folder())
      layer_select = parent;

    // select other layer
    set_current_layer(layer_select);
  }

  // remove the layer
  if (is_enabled())
    undo_remove_layer(m_sprite->undo, layer);

  parent->remove_layer(layer);

  // destroy the layer
  delete layer;
}

void Undoable::move_layer_after(Layer* layer, Layer* after_this)
{
  if (is_enabled())
    undo_move_layer(m_sprite->undo, layer);

  layer->get_parent()->move_layer(layer, after_this);
}

void Undoable::crop_layer(Layer* layer, int x, int y, int w, int h, int bgcolor)
{
  if (!layer->is_image())
    return;

  if (!layer->is_background())
    bgcolor = 0;

  CelIterator it = ((LayerImage*)layer)->get_cel_begin();
  CelIterator end = ((LayerImage*)layer)->get_cel_end();
  for (; it != end; ++it)
    crop_cel(*it, x, y, w, h, bgcolor);
}

/**
 * Moves every frame in @a layer with the offset (@a dx, @a dy).
 */
void Undoable::displace_layers(Layer* layer, int dx, int dy)
{
  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE: {
      CelIterator it = ((LayerImage*)layer)->get_cel_begin();
      CelIterator end = ((LayerImage*)layer)->get_cel_end();
      for (; it != end; ++it) {
	Cel* cel = *it;
	set_cel_position(cel, cel->x+dx, cel->y+dy);
      }
      break;
    }

    case GFXOBJ_LAYER_FOLDER: {
      LayerIterator it = ((LayerFolder*)layer)->get_layer_begin();
      LayerIterator end = ((LayerFolder*)layer)->get_layer_end();
      for (; it != end; ++it)
	displace_layers(*it, dx, dy);
      break;
    }

  }
}

void Undoable::background_from_layer(LayerImage* layer, int bgcolor)
{
  assert(layer);
  assert(layer->is_image());
  assert(layer->is_readable());
  assert(layer->is_writable());
  assert(layer->getSprite() == m_sprite);
  assert(sprite_get_background_layer(m_sprite) == NULL);

  // create a temporary image to draw each frame of the new
  // `Background' layer
  std::auto_ptr<Image> bg_image_wrap(image_new(m_sprite->imgtype, m_sprite->w, m_sprite->h));
  Image* bg_image = bg_image_wrap.get();

  CelIterator it = layer->get_cel_begin();
  CelIterator end = layer->get_cel_end();

  for (; it != end; ++it) {
    Cel* cel = *it;
    assert((cel->image > 0) &&
	   (cel->image < m_sprite->stock->nimage));

    // get the image from the sprite's stock of images
    Image* cel_image = stock_get_image(m_sprite->stock, cel->image);
    assert(cel_image);

    image_clear(bg_image, bgcolor);
    image_merge(bg_image, cel_image,
		cel->x,
		cel->y,
		MID(0, cel->opacity, 255),
		layer->get_blend_mode());

    // now we have to copy the new image (bg_image) to the cel...
    set_cel_position(cel, 0, 0);

    // same size of cel-image and bg-image
    if (bg_image->w == cel_image->w &&
	bg_image->h == cel_image->h) {
      if (is_enabled())
	undo_image(m_sprite->undo, cel_image, 0, 0, cel_image->w, cel_image->h);

      image_copy(cel_image, bg_image, 0, 0);
    }
    else {
      replace_stock_image(cel->image, image_new_copy(bg_image));
    }
  }

  // Fill all empty cels with a flat-image filled with bgcolor
  for (int frame=0; frame<m_sprite->frames; frame++) {
    Cel* cel = layer->get_cel(frame);
    if (!cel) {
      Image* cel_image = image_new(m_sprite->imgtype, m_sprite->w, m_sprite->h);
      image_clear(cel_image, bgcolor);

      // Add the new image in the stock
      int image_index = add_image_in_stock(cel_image);

      // Create the new cel and add it to the new background layer
      cel = cel_new(frame, image_index);
      add_cel(layer, cel);
    }
  }

  configure_layer_as_background(layer);
}

void Undoable::layer_from_background()
{
  assert(sprite_get_background_layer(m_sprite) != NULL);
  assert(m_sprite->layer != NULL);
  assert(m_sprite->layer->is_image());
  assert(m_sprite->layer->is_readable());
  assert(m_sprite->layer->is_writable());
  assert(m_sprite->layer->is_background());

  if (is_enabled()) {
    undo_data(m_sprite->undo,
	      m_sprite->layer,
	      m_sprite->layer->flags_addr(),
	      sizeof(*m_sprite->layer->flags_addr()));

    undo_set_layer_name(m_sprite->undo, m_sprite->layer);
  }

  m_sprite->layer->set_background(false);
  m_sprite->layer->set_moveable(true);
  m_sprite->layer->set_name("Layer 0");
}

void Undoable::flatten_layers(int bgcolor)
{
  Image* cel_image;
  Cel* cel;
  int frame;

  // create a temporary image
  std::auto_ptr<Image> image_wrap(image_new(m_sprite->imgtype, m_sprite->w, m_sprite->h));
  Image* image = image_wrap.get();

  /* get the background layer from the sprite */
  LayerImage* background = sprite_get_background_layer(m_sprite);
  if (!background) {
    /* if there aren't a background layer we must to create the background */
    background = new LayerImage(m_sprite);

    if (is_enabled())
      undo_add_layer(m_sprite->undo, m_sprite->get_folder(), background);

    m_sprite->get_folder()->add_layer(background);

    if (is_enabled())
      undo_move_layer(m_sprite->undo, background);
    
    background->configure_as_background();
  }

  /* copy all frames to the background */
  for (frame=0; frame<m_sprite->frames; frame++) {
    /* clear the image and render this frame */
    image_clear(image, bgcolor);
    layer_render(m_sprite->get_folder(), image, 0, 0, frame);

    cel = background->get_cel(frame);
    if (cel) {
      cel_image = m_sprite->stock->image[cel->image];
      assert(cel_image != NULL);

      /* we have to save the current state of `cel_image' in the undo */
      if (is_enabled()) {
	Dirty* dirty = dirty_new_from_differences(cel_image, image);
	dirty_save_image_data(dirty);
	undo_dirty(m_sprite->undo, dirty);
      }
    }
    else {
      /* if there aren't a cel in this frame in the background, we
	 have to create a copy of the image for the new cel */
      cel_image = image_new_copy(image);
      /* TODO error handling: if (!cel_image) { ... } */

      /* here we create the new cel (with the new image `cel_image') */
      cel = cel_new(frame, stock_add_image(m_sprite->stock, cel_image));
      /* TODO error handling: if (!cel) { ... } */

      /* and finally we add the cel in the background */
      background->add_cel(cel);
    }

    image_copy(cel_image, image, 0, 0);
  }

  /* select the background */
  if (m_sprite->layer != background) {
    if (is_enabled())
      undo_set_layer(m_sprite->undo, m_sprite);

    sprite_set_layer(m_sprite, background);
  }

  /* remove old layers */
  LayerList layers = m_sprite->get_folder()->get_layers_list();
  LayerIterator it = layers.begin();
  LayerIterator end = layers.end();

  for (; it != end; ++it) {
    if (*it != background) {
      Layer* old_layer = *it;

      // remove the layer
      if (is_enabled())
	undo_remove_layer(m_sprite->undo, old_layer);

      m_sprite->get_folder()->remove_layer(old_layer);

      // destroy the layer
      delete old_layer;
    }
  }
}

void Undoable::configure_layer_as_background(LayerImage* layer)
{
  if (is_enabled()) {
    undo_data(m_sprite->undo, layer, layer->flags_addr(), sizeof(*layer->flags_addr()));
    undo_set_layer_name(m_sprite->undo, layer);
    undo_move_layer(m_sprite->undo, layer);
  }

  layer->configure_as_background();
}

void Undoable::new_frame()
{
  // add a new cel to every layer
  new_frame_for_layer(m_sprite->get_folder(),
		      m_sprite->frame+1);

  // increment frames counter in the sprite
  set_number_of_frames(m_sprite->frames+1);

  // go to next frame (the new one)
  set_current_frame(m_sprite->frame+1);
}

void Undoable::new_frame_for_layer(Layer* layer, int frame)
{
  assert(layer);
  assert(frame >= 0);

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE:
      // displace all cels in '>=frame' to the next frame
      for (int c=m_sprite->frames-1; c>=frame; --c) {
	Cel* cel = static_cast<LayerImage*>(layer)->get_cel(c);
	if (cel)
	  set_cel_frame_position(cel, cel->frame+1);
      }

      copy_previous_frame(layer, frame);
      break;

    case GFXOBJ_LAYER_FOLDER: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->get_layer_begin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it)
	new_frame_for_layer(*it, frame);
      break;
    }

  }
}

void Undoable::remove_frame(int frame)
{
  assert(frame >= 0);

  // remove cels from this frame (and displace one position backward
  // all next frames)
  remove_frame_of_layer(m_sprite->get_folder(), frame);

  /* decrement frames counter in the sprite */
  if (is_enabled())
    undo_set_frames(m_sprite->undo, m_sprite);

  sprite_set_frames(m_sprite, m_sprite->frames-1);

  /* move backward if we are outside the range of frames */
  if (m_sprite->frame >= m_sprite->frames) {
    if (is_enabled())
      undo_int(m_sprite->undo, m_sprite, &m_sprite->frame);

    sprite_set_frame(m_sprite, m_sprite->frames-1);
  }
}

void Undoable::remove_frame_of_layer(Layer* layer, int frame)
{
  assert(layer);
  assert(frame >= 0);

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE:
      if (Cel* cel = static_cast<LayerImage*>(layer)->get_cel(frame))
	remove_cel(static_cast<LayerImage*>(layer), cel);

      for (++frame; frame<m_sprite->frames; ++frame)
	if (Cel* cel = static_cast<LayerImage*>(layer)->get_cel(frame))
	  set_cel_frame_position(cel, cel->frame-1);
      break;

    case GFXOBJ_LAYER_FOLDER: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->get_layer_begin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it)
	remove_frame_of_layer(*it, frame);
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
  Cel* src_cel = static_cast<LayerImage*>(layer)->get_cel(frame-1);
  Image* src_image = src_cel ? stock_get_image(m_sprite->stock,
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
  add_cel(static_cast<LayerImage*>(layer), dst_cel);
}

void Undoable::add_cel(LayerImage* layer, Cel* cel)
{
  assert(layer);
  assert(cel);

  if (is_enabled())
    undo_add_cel(m_sprite->undo, layer, cel);

  layer->add_cel(cel);
}

void Undoable::remove_cel(LayerImage* layer, Cel* cel)
{
  assert(layer);
  assert(cel);

  // find if the image that use the cel to remove, is used by
  // another cels
  bool used = false;
  for (int frame=0; frame<m_sprite->frames; ++frame) {
    Cel* it = layer->get_cel(frame);
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
    undo_remove_cel(m_sprite->undo, layer, cel);

  // remove the cel from the layer
  layer->remove_cel(cel);

  // and here we destroy the cel
  cel_free(cel);
}

void Undoable::set_cel_frame_position(Cel* cel, int frame)
{
  assert(cel);
  assert(frame >= 0);

  if (is_enabled())
    undo_int(m_sprite->undo, cel, &cel->frame);

  cel->frame = frame;
}

void Undoable::set_cel_position(Cel* cel, int x, int y)
{
  assert(cel);

  if (is_enabled()) {
    undo_int(m_sprite->undo, cel, &cel->x);
    undo_int(m_sprite->undo, cel, &cel->y);
  }

  cel->x = x;
  cel->y = y;
}

void Undoable::set_frame_duration(int frame, int msecs)
{
  if (is_enabled())
    undo_set_frlen(m_sprite->undo, m_sprite, frame);

  sprite_set_frlen(m_sprite, frame, msecs);
}

void Undoable::set_constant_frame_rate(int msecs)
{
  if (is_enabled()) {
    for (int fr=0; fr<m_sprite->frames; ++fr)
      undo_set_frlen(m_sprite->undo, m_sprite, fr);
  }

  sprite_set_speed(m_sprite, msecs);
}

void Undoable::move_frame_before(int frame, int before_frame)
{
  if (frame != before_frame &&
      frame >= 0 &&
      frame < m_sprite->frames &&
      before_frame >= 0 &&
      before_frame < m_sprite->frames) {
    // change the frame-lengths...

    int frlen_aux = m_sprite->frlens[frame];

    // moving the frame to the future
    if (frame < before_frame) {
      frlen_aux = m_sprite->frlens[frame];

      for (int c=frame; c<before_frame-1; c++)
	set_frame_duration(c, m_sprite->frlens[c+1]);

      set_frame_duration(before_frame-1, frlen_aux);
    }
    // moving the frame to the past
    else if (before_frame < frame) {
      frlen_aux = m_sprite->frlens[frame];

      for (int c=frame; c>before_frame; c--)
	set_frame_duration(c, m_sprite->frlens[c-1]);

      set_frame_duration(before_frame, frlen_aux);
    }

    // change the cels of position...
    move_frame_before_layer(m_sprite->get_folder(), frame, before_frame);
  }
}

void Undoable::move_frame_before_layer(Layer* layer, int frame, int before_frame)
{
  assert(layer);

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE: {
      CelIterator it = ((LayerImage*)layer)->get_cel_begin();
      CelIterator end = ((LayerImage*)layer)->get_cel_end();

      for (; it != end; ++it) {
	Cel* cel = *it;
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

    case GFXOBJ_LAYER_FOLDER: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->get_layer_begin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it)
	move_frame_before_layer(*it, frame, before_frame);
      break;
    }

  }
}

Cel* Undoable::get_current_cel()
{
  if (m_sprite->layer && m_sprite->layer->is_image())
    return static_cast<LayerImage*>(m_sprite->layer)->get_cel(m_sprite->frame);
  else
    return NULL;
}

void Undoable::crop_cel(Cel* cel, int x, int y, int w, int h, int bgcolor)
{
  Image* cel_image = stock_get_image(m_sprite->stock, cel->image);
  assert(cel_image);
    
  // create the new image through a crop
  Image* new_image = image_crop(cel_image, x-cel->x, y-cel->y, w, h, bgcolor);

  // replace the image in the stock that is pointed by the cel
  replace_stock_image(cel->image, new_image);

  // update the cel's position
  set_cel_position(cel, x, y);
}

Image* Undoable::get_cel_image(Cel* cel)
{
  if (cel && cel->image >= 0 && cel->image < m_sprite->stock->nimage)
    return m_sprite->stock->image[cel->image];
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
  if (m_sprite->mask->is_empty()) {
    // if the layer is the background then we clear the image
    if (m_sprite->layer->is_background()) {
      if (is_enabled())
	undo_image(m_sprite->undo, image, 0, 0, image->w, image->h);

      // clear all
      image_clear(image, bgcolor);
    }
    // if the layer is transparent we can remove the cel (and its
    // associated image)
    else {
      remove_cel(static_cast<LayerImage*>(m_sprite->layer), cel);
    }
  }
  else {
    int u, v, putx, puty;
    int x1 = MAX(0, m_sprite->mask->x);
    int y1 = MAX(0, m_sprite->mask->y);
    int x2 = MIN(image->w-1, m_sprite->mask->x+m_sprite->mask->w-1);
    int y2 = MIN(image->h-1, m_sprite->mask->y+m_sprite->mask->h-1);

    // do nothing
    if (x1 > x2 || y1 > y2)
      return;

    if (is_enabled())
      undo_image(m_sprite->undo, image, x1, y1, x2-x1+1, y2-y1+1);

    // clear the masked zones
    for (v=0; v<m_sprite->mask->h; v++) {
      div_t d = div(0, 8);
      ase_uint8* address = ((ase_uint8 **)m_sprite->mask->bitmap->line)[v]+d.quot;

      for (u=0; u<m_sprite->mask->w; u++) {
	if ((*address & (1<<d.rem))) {
	  putx = u + m_sprite->mask->x - cel->x;
	  puty = v + m_sprite->mask->y - cel->y;
	  image_putpixel(image, putx, puty, bgcolor);
	}

	_image_bitmap_next_bit(d, address);
      }
    }
  }
}

void Undoable::flip_image(Image* image, int x1, int y1, int x2, int y2,
			  bool flip_horizontal, bool flip_vertical)
{
  // insert the undo operation
  if (is_enabled()) {
    if (flip_horizontal)
      undo_flip(m_sprite->undo, image, x1, y1, x2, y2, true);

    if (flip_vertical)
      undo_flip(m_sprite->undo, image, x1, y1, x2, y2, false);
  }

  // flip the portion of the bitmap
  Image* area = image_crop(image, x1, y1, x2-x1+1, y2-y1+1, 0);
  int x, y;

  for (y=0; y<(y2-y1+1); y++)
    for (x=0; x<(x2-x1+1); x++)
      image_putpixel(image,
		     flip_horizontal ? x2-x: x1+x,
		     flip_vertical ? y2-y: y1+y,
		     image_getpixel(area, x, y));

  image_free(area);
}

void Undoable::copy_to_current_mask(Mask* mask)
{
  assert(m_sprite->mask);
  assert(mask);

  if (is_enabled())
    undo_set_mask(m_sprite->undo, m_sprite);

  mask_copy(m_sprite->mask, mask);
}

void Undoable::set_mask_position(int x, int y)
{
  assert(m_sprite->mask);

  if (is_enabled()) {
    undo_int(m_sprite->undo, m_sprite->mask, &m_sprite->mask->x);
    undo_int(m_sprite->undo, m_sprite->mask, &m_sprite->mask->y);
  }

  m_sprite->mask->x = x;
  m_sprite->mask->y = y;
}

void Undoable::deselect_mask()
{
  // Destroy the *deselected* mask
  Mask* mask = sprite_request_mask(m_sprite, "*deselected*");
  if (mask) {
    sprite_remove_mask(m_sprite, mask);
    mask_free(mask);
  }

  // Save the selection in the repository
  mask = mask_new_copy(m_sprite->mask);
  mask_set_name(mask, "*deselected*");
  sprite_add_mask(m_sprite, mask);

  if (undo_is_enabled(m_sprite->undo))
    undo_set_mask(m_sprite->undo, m_sprite);

  /// Deselect the mask
  mask_none(m_sprite->mask);
}
