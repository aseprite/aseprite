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
  ASSERT(label != NULL);

  m_sprite = sprite;
  m_committed = false;
  m_enabled_flag = undo_is_enabled(m_sprite->getUndo());

  if (is_enabled()) {
    undo_set_label(m_sprite->getUndo(), label);
    undo_open(m_sprite->getUndo());
  }
}

Undoable::~Undoable()
{
  if (is_enabled()) {
    // close the undo information
    undo_close(m_sprite->getUndo());

    // if it isn't committed, we have to rollback all changes
    if (!m_committed) {
      // undo the group of operations
      undo_do_undo(m_sprite->getUndo());

      // clear the redo (sorry to the user, here we lost the old redo
      // information)
      undo_clear_redo(m_sprite->getUndo());
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
  ASSERT(frames >= 1);

  // Save in undo the current totalFrames property
  if (is_enabled())
    undo_set_frames(m_sprite->getUndo(), m_sprite);

  // Change the property
  m_sprite->setTotalFrames(frames);
}

void Undoable::set_current_frame(int frame)
{
  ASSERT(frame >= 0);

  if (is_enabled())
    undo_set_frame(m_sprite->getUndo(), m_sprite);

  m_sprite->setCurrentFrame(frame);
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
    undo_set_layer(m_sprite->getUndo(), m_sprite);

  m_sprite->setCurrentLayer(layer);
}

void Undoable::set_sprite_size(int w, int h)
{
  ASSERT(w > 0);
  ASSERT(h > 0);

  if (is_enabled())
    undo_set_size(m_sprite->getUndo(), m_sprite);

  m_sprite->setSize(w, h);
}

void Undoable::crop_sprite(int x, int y, int w, int h, int bgcolor)
{
  set_sprite_size(w, h);

  displace_layers(m_sprite->getFolder(), -x, -y);

  Layer *background_layer = m_sprite->getBackgroundLayer();
  if (background_layer)
    crop_layer(background_layer, 0, 0, m_sprite->getWidth(), m_sprite->getHeight(), bgcolor);

  if (!m_sprite->getMask()->is_empty())
    set_mask_position(m_sprite->getMask()->x-x, m_sprite->getMask()->y-y);
}

void Undoable::autocrop_sprite(int bgcolor)
{
  int old_frame = m_sprite->getCurrentFrame();
  int x1, y1, x2, y2;
  int u1, v1, u2, v2;

  x1 = y1 = INT_MAX;
  x2 = y2 = INT_MIN;

  Image* image = image_new(m_sprite->getImgType(),
			   m_sprite->getWidth(),
			   m_sprite->getHeight());

  for (int frame=0; frame<m_sprite->getTotalFrames(); ++frame) {
    m_sprite->setCurrentFrame(frame);

    image_clear(image, 0);
    m_sprite->render(image, 0, 0);

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
  m_sprite->setCurrentFrame(old_frame);

  image_free(image);

  // do nothing
  if (x1 > x2 || y1 > y2)
    return;

  crop_sprite(x1, y1, x2-x1+1, y2-y1+1, bgcolor);
}

void Undoable::set_imgtype(int new_imgtype, int dithering_method)
{
  Image *old_image;
  Image *new_image;
  int c;

  if (m_sprite->getImgType() == new_imgtype)
    return;

  /* change imgtype of the stock of images */
  if (is_enabled())
    undo_int(m_sprite->getUndo(), m_sprite->getStock(), &m_sprite->getStock()->imgtype);

  m_sprite->getStock()->imgtype = new_imgtype;

  // Use the rgbmap for the specified sprite
  const RgbMap* rgbmap = m_sprite->getRgbMap();

  for (c=0; c<m_sprite->getStock()->nimage; c++) {
    old_image = stock_get_image(m_sprite->getStock(), c);
    if (!old_image)
      continue;

    new_image = image_set_imgtype(old_image, new_imgtype, dithering_method, rgbmap,
				  // TODO check this out
				  m_sprite->getCurrentPalette());
    if (!new_image)
      return;		/* TODO error handling: not enough memory!
			   we should undo all work done */

    if (is_enabled())
      undo_replace_image(m_sprite->getUndo(), m_sprite->getStock(), c);

    image_free(old_image);
    stock_replace_image(m_sprite->getStock(), c, new_image);
  }

  /* change "sprite.imgtype" field */
  if (is_enabled())
    undo_set_imgtype(m_sprite->getUndo(), m_sprite);

  m_sprite->setImgType(new_imgtype);

  // change "sprite.palette"
  if (new_imgtype == IMAGE_GRAYSCALE) {
    if (is_enabled()) {
      Palette* palette;
      JLink link;

      // Save all palettes
      JI_LIST_FOR_EACH(m_sprite->getPalettes(), link) {
	palette = reinterpret_cast<Palette*>(link->data);
	undo_remove_palette(m_sprite->getUndo(), m_sprite, palette);
      }
    }

    std::auto_ptr<Palette> graypal(Palette::createGrayscale());

    m_sprite->resetPalettes();
    m_sprite->setPalette(graypal.get(), true);
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
  ASSERT(image);

  // add the image in the stock
  int image_index = stock_add_image(m_sprite->getStock(), image);

  if (is_enabled())
    undo_add_image(m_sprite->getUndo(), m_sprite->getStock(), image_index);

  return image_index;
}

/**
 * Removes and destroys the specified image in the stock.
 */
void Undoable::remove_image_from_stock(int image_index)
{
  ASSERT(image_index >= 0);

  Image* image = stock_get_image(m_sprite->getStock(), image_index);
  ASSERT(image);

  if (is_enabled())
    undo_remove_image(m_sprite->getUndo(), m_sprite->getStock(), image_index);

  stock_remove_image(m_sprite->getStock(), image);
  image_free(image);
}

void Undoable::replace_stock_image(int image_index, Image* new_image)
{
  // get the current image in the 'image_index' position
  Image* old_image = stock_get_image(m_sprite->getStock(), image_index);
  ASSERT(old_image);

  // replace the image in the stock
  if (is_enabled())
    undo_replace_image(m_sprite->getUndo(), m_sprite->getStock(), image_index);

  stock_replace_image(m_sprite->getStock(), image_index, new_image);

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
    undo_add_layer(m_sprite->getUndo(), m_sprite->getFolder(), layer);

  m_sprite->getFolder()->add_layer(layer);

  // select the new layer
  set_current_layer(layer);

  return layer;
}

/**
 * Removes and destroys the specified layer.
 */
void Undoable::remove_layer(Layer* layer)
{
  ASSERT(layer);

  LayerFolder* parent = layer->get_parent();

  // if the layer to be removed is the selected layer
  if (layer == m_sprite->getCurrentLayer()) {
    Layer* layer_select = NULL;

    // select: previous layer, or next layer, or parent(if it is not the
    // main layer of sprite set)
    if (layer->get_prev())
      layer_select = layer->get_prev();
    else if (layer->get_next())
      layer_select = layer->get_next();
    else if (parent != m_sprite->getFolder())
      layer_select = parent;

    // select other layer
    set_current_layer(layer_select);
  }

  // remove the layer
  if (is_enabled())
    undo_remove_layer(m_sprite->getUndo(), layer);

  parent->remove_layer(layer);

  // destroy the layer
  delete layer;
}

void Undoable::move_layer_after(Layer* layer, Layer* after_this)
{
  if (is_enabled())
    undo_move_layer(m_sprite->getUndo(), layer);

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
  ASSERT(layer);
  ASSERT(layer->is_image());
  ASSERT(layer->is_readable());
  ASSERT(layer->is_writable());
  ASSERT(layer->getSprite() == m_sprite);
  ASSERT(m_sprite->getBackgroundLayer() == NULL);

  // create a temporary image to draw each frame of the new
  // `Background' layer
  std::auto_ptr<Image> bg_image_wrap(image_new(m_sprite->getImgType(),
					       m_sprite->getWidth(),
					       m_sprite->getHeight()));
  Image* bg_image = bg_image_wrap.get();

  CelIterator it = layer->get_cel_begin();
  CelIterator end = layer->get_cel_end();

  for (; it != end; ++it) {
    Cel* cel = *it;
    ASSERT((cel->image > 0) &&
	   (cel->image < m_sprite->getStock()->nimage));

    // get the image from the sprite's stock of images
    Image* cel_image = stock_get_image(m_sprite->getStock(), cel->image);
    ASSERT(cel_image);

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
	undo_image(m_sprite->getUndo(), cel_image, 0, 0, cel_image->w, cel_image->h);

      image_copy(cel_image, bg_image, 0, 0);
    }
    else {
      replace_stock_image(cel->image, image_new_copy(bg_image));
    }
  }

  // Fill all empty cels with a flat-image filled with bgcolor
  for (int frame=0; frame<m_sprite->getTotalFrames(); frame++) {
    Cel* cel = layer->get_cel(frame);
    if (!cel) {
      Image* cel_image = image_new(m_sprite->getImgType(), m_sprite->getWidth(), m_sprite->getHeight());
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
  ASSERT(m_sprite->getBackgroundLayer() != NULL);
  ASSERT(m_sprite->getCurrentLayer() != NULL);
  ASSERT(m_sprite->getCurrentLayer()->is_image());
  ASSERT(m_sprite->getCurrentLayer()->is_readable());
  ASSERT(m_sprite->getCurrentLayer()->is_writable());
  ASSERT(m_sprite->getCurrentLayer()->is_background());

  if (is_enabled()) {
    undo_data(m_sprite->getUndo(),
	      m_sprite->getCurrentLayer(),
	      m_sprite->getCurrentLayer()->flags_addr(),
	      sizeof(*m_sprite->getCurrentLayer()->flags_addr()));

    undo_set_layer_name(m_sprite->getUndo(), m_sprite->getCurrentLayer());
  }

  m_sprite->getCurrentLayer()->set_background(false);
  m_sprite->getCurrentLayer()->set_moveable(true);
  m_sprite->getCurrentLayer()->set_name("Layer 0");
}

void Undoable::flatten_layers(int bgcolor)
{
  Image* cel_image;
  Cel* cel;
  int frame;

  // create a temporary image
  std::auto_ptr<Image> image_wrap(image_new(m_sprite->getImgType(),
					    m_sprite->getWidth(),
					    m_sprite->getHeight()));
  Image* image = image_wrap.get();

  /* get the background layer from the sprite */
  LayerImage* background = m_sprite->getBackgroundLayer();
  if (!background) {
    /* if there aren't a background layer we must to create the background */
    background = new LayerImage(m_sprite);

    if (is_enabled())
      undo_add_layer(m_sprite->getUndo(), m_sprite->getFolder(), background);

    m_sprite->getFolder()->add_layer(background);

    if (is_enabled())
      undo_move_layer(m_sprite->getUndo(), background);
    
    background->configure_as_background();
  }

  /* copy all frames to the background */
  for (frame=0; frame<m_sprite->getTotalFrames(); frame++) {
    /* clear the image and render this frame */
    image_clear(image, bgcolor);
    layer_render(m_sprite->getFolder(), image, 0, 0, frame);

    cel = background->get_cel(frame);
    if (cel) {
      cel_image = m_sprite->getStock()->image[cel->image];
      ASSERT(cel_image != NULL);

      /* we have to save the current state of `cel_image' in the undo */
      if (is_enabled()) {
	Dirty* dirty = dirty_new_from_differences(cel_image, image);
	dirty_save_image_data(dirty);
	undo_dirty(m_sprite->getUndo(), dirty);
      }
    }
    else {
      /* if there aren't a cel in this frame in the background, we
	 have to create a copy of the image for the new cel */
      cel_image = image_new_copy(image);
      /* TODO error handling: if (!cel_image) { ... } */

      /* here we create the new cel (with the new image `cel_image') */
      cel = cel_new(frame, stock_add_image(m_sprite->getStock(), cel_image));
      /* TODO error handling: if (!cel) { ... } */

      /* and finally we add the cel in the background */
      background->add_cel(cel);
    }

    image_copy(cel_image, image, 0, 0);
  }

  /* select the background */
  if (m_sprite->getCurrentLayer() != background) {
    if (is_enabled())
      undo_set_layer(m_sprite->getUndo(), m_sprite);

    m_sprite->setCurrentLayer(background);
  }

  /* remove old layers */
  LayerList layers = m_sprite->getFolder()->get_layers_list();
  LayerIterator it = layers.begin();
  LayerIterator end = layers.end();

  for (; it != end; ++it) {
    if (*it != background) {
      Layer* old_layer = *it;

      // remove the layer
      if (is_enabled())
	undo_remove_layer(m_sprite->getUndo(), old_layer);

      m_sprite->getFolder()->remove_layer(old_layer);

      // destroy the layer
      delete old_layer;
    }
  }
}

void Undoable::configure_layer_as_background(LayerImage* layer)
{
  if (is_enabled()) {
    undo_data(m_sprite->getUndo(), layer, layer->flags_addr(), sizeof(*layer->flags_addr()));
    undo_set_layer_name(m_sprite->getUndo(), layer);
    undo_move_layer(m_sprite->getUndo(), layer);
  }

  layer->configure_as_background();
}

void Undoable::new_frame()
{
  // add a new cel to every layer
  new_frame_for_layer(m_sprite->getFolder(),
		      m_sprite->getCurrentFrame()+1);

  // increment frames counter in the sprite
  set_number_of_frames(m_sprite->getTotalFrames()+1);

  // go to next frame (the new one)
  set_current_frame(m_sprite->getCurrentFrame()+1);
}

void Undoable::new_frame_for_layer(Layer* layer, int frame)
{
  ASSERT(layer);
  ASSERT(frame >= 0);

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE:
      // displace all cels in '>=frame' to the next frame
      for (int c=m_sprite->getTotalFrames()-1; c>=frame; --c) {
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
  ASSERT(frame >= 0);

  // Remove cels from this frame (and displace one position backward
  // all next frames)
  remove_frame_of_layer(m_sprite->getFolder(), frame);

  // New value for totalFrames propety
  int newTotalFrames = m_sprite->getTotalFrames()-1;

  // Move backward if we will be outside the range of frames
  if (m_sprite->getCurrentFrame() >= newTotalFrames)
    set_current_frame(newTotalFrames-1);

  // Decrement frames counter in the sprite
  set_number_of_frames(newTotalFrames);
}

void Undoable::remove_frame_of_layer(Layer* layer, int frame)
{
  ASSERT(layer);
  ASSERT(frame >= 0);

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE:
      if (Cel* cel = static_cast<LayerImage*>(layer)->get_cel(frame))
	remove_cel(static_cast<LayerImage*>(layer), cel);

      for (++frame; frame<m_sprite->getTotalFrames(); ++frame)
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
  ASSERT(layer);
  ASSERT(frame > 0);

  // create a copy of the previous cel
  Cel* src_cel = static_cast<LayerImage*>(layer)->get_cel(frame-1);
  Image* src_image = src_cel ? stock_get_image(m_sprite->getStock(),
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
  ASSERT(layer);
  ASSERT(cel);

  if (is_enabled())
    undo_add_cel(m_sprite->getUndo(), layer, cel);

  layer->add_cel(cel);
}

void Undoable::remove_cel(LayerImage* layer, Cel* cel)
{
  ASSERT(layer);
  ASSERT(cel);

  // find if the image that use the cel to remove, is used by
  // another cels
  bool used = false;
  for (int frame=0; frame<m_sprite->getTotalFrames(); ++frame) {
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
    undo_remove_cel(m_sprite->getUndo(), layer, cel);

  // remove the cel from the layer
  layer->remove_cel(cel);

  // and here we destroy the cel
  cel_free(cel);
}

void Undoable::set_cel_frame_position(Cel* cel, int frame)
{
  ASSERT(cel);
  ASSERT(frame >= 0);

  if (is_enabled())
    undo_int(m_sprite->getUndo(), cel, &cel->frame);

  cel->frame = frame;
}

void Undoable::set_cel_position(Cel* cel, int x, int y)
{
  ASSERT(cel);

  if (is_enabled()) {
    undo_int(m_sprite->getUndo(), cel, &cel->x);
    undo_int(m_sprite->getUndo(), cel, &cel->y);
  }

  cel->x = x;
  cel->y = y;
}

void Undoable::set_frame_duration(int frame, int msecs)
{
  if (is_enabled())
    undo_set_frlen(m_sprite->getUndo(), m_sprite, frame);

  m_sprite->setFrameDuration(frame, msecs);
}

void Undoable::set_constant_frame_rate(int msecs)
{
  if (is_enabled()) {
    for (int fr=0; fr<m_sprite->getTotalFrames(); ++fr)
      undo_set_frlen(m_sprite->getUndo(), m_sprite, fr);
  }

  m_sprite->setDurationForAllFrames(msecs);
}

void Undoable::move_frame_before(int frame, int before_frame)
{
  if (frame != before_frame &&
      frame >= 0 &&
      frame < m_sprite->getTotalFrames() &&
      before_frame >= 0 &&
      before_frame < m_sprite->getTotalFrames()) {
    // change the frame-lengths...

    int frlen_aux = m_sprite->getFrameDuration(frame);

    // moving the frame to the future
    if (frame < before_frame) {
      for (int c=frame; c<before_frame-1; c++)
	set_frame_duration(c, m_sprite->getFrameDuration(c+1));

      set_frame_duration(before_frame-1, frlen_aux);
    }
    // moving the frame to the past
    else if (before_frame < frame) {
      for (int c=frame; c>before_frame; c--)
	set_frame_duration(c, m_sprite->getFrameDuration(c-1));

      set_frame_duration(before_frame, frlen_aux);
    }

    // change the cels of position...
    move_frame_before_layer(m_sprite->getFolder(), frame, before_frame);
  }
}

void Undoable::move_frame_before_layer(Layer* layer, int frame, int before_frame)
{
  ASSERT(layer);

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
  if (m_sprite->getCurrentLayer() && m_sprite->getCurrentLayer()->is_image())
    return static_cast<LayerImage*>(m_sprite->getCurrentLayer())->get_cel(m_sprite->getCurrentFrame());
  else
    return NULL;
}

void Undoable::crop_cel(Cel* cel, int x, int y, int w, int h, int bgcolor)
{
  Image* cel_image = stock_get_image(m_sprite->getStock(), cel->image);
  ASSERT(cel_image);
    
  // create the new image through a crop
  Image* new_image = image_crop(cel_image, x-cel->x, y-cel->y, w, h, bgcolor);

  // replace the image in the stock that is pointed by the cel
  replace_stock_image(cel->image, new_image);

  // update the cel's position
  set_cel_position(cel, x, y);
}

Image* Undoable::get_cel_image(Cel* cel)
{
  if (cel && cel->image >= 0 && cel->image < m_sprite->getStock()->nimage)
    return m_sprite->getStock()->image[cel->image];
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
  if (m_sprite->getMask()->is_empty()) {
    // if the layer is the background then we clear the image
    if (m_sprite->getCurrentLayer()->is_background()) {
      if (is_enabled())
	undo_image(m_sprite->getUndo(), image, 0, 0, image->w, image->h);

      // clear all
      image_clear(image, bgcolor);
    }
    // if the layer is transparent we can remove the cel (and its
    // associated image)
    else {
      remove_cel(static_cast<LayerImage*>(m_sprite->getCurrentLayer()), cel);
    }
  }
  else {
    int offset_x = m_sprite->getMask()->x-cel->x;
    int offset_y = m_sprite->getMask()->y-cel->y;
    int u, v, putx, puty;
    int x1 = MAX(0, offset_x);
    int y1 = MAX(0, offset_y);
    int x2 = MIN(image->w-1, offset_x+m_sprite->getMask()->w-1);
    int y2 = MIN(image->h-1, offset_y+m_sprite->getMask()->h-1);

    // do nothing
    if (x1 > x2 || y1 > y2)
      return;

    if (is_enabled())
      undo_image(m_sprite->getUndo(), image, x1, y1, x2-x1+1, y2-y1+1);

    // clear the masked zones
    for (v=0; v<m_sprite->getMask()->h; v++) {
      div_t d = div(0, 8);
      ase_uint8* address = ((ase_uint8 **)m_sprite->getMask()->bitmap->line)[v]+d.quot;

      for (u=0; u<m_sprite->getMask()->w; u++) {
	if ((*address & (1<<d.rem))) {
	  putx = u + offset_x;
	  puty = v + offset_y;
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
      undo_flip(m_sprite->getUndo(), image, x1, y1, x2, y2, true);

    if (flip_vertical)
      undo_flip(m_sprite->getUndo(), image, x1, y1, x2, y2, false);
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

void Undoable::paste_image(const Image* src_image, int x, int y, int opacity)
{
  const Layer* layer = m_sprite->getCurrentLayer();

  ASSERT(layer);
  ASSERT(layer->is_image());
  ASSERT(layer->is_readable());
  ASSERT(layer->is_writable());

  Cel* cel = ((LayerImage*)layer)->get_cel(m_sprite->getCurrentFrame());
  ASSERT(cel);

  Image* cel_image = stock_get_image(m_sprite->getStock(), cel->image);
  Image* cel_image2 = image_new_copy(cel_image);
  image_merge(cel_image2, src_image, x-cel->x, y-cel->y, opacity, BLEND_MODE_NORMAL);

  replace_stock_image(cel->image, cel_image2); // TODO fix this, improve, avoid replacing the whole image
}

void Undoable::copy_to_current_mask(Mask* mask)
{
  ASSERT(m_sprite->getMask());
  ASSERT(mask);

  if (is_enabled())
    undo_set_mask(m_sprite->getUndo(), m_sprite);

  mask_copy(m_sprite->getMask(), mask);
}

void Undoable::set_mask_position(int x, int y)
{
  ASSERT(m_sprite->getMask());

  if (is_enabled()) {
    undo_int(m_sprite->getUndo(), m_sprite->getMask(), &m_sprite->getMask()->x);
    undo_int(m_sprite->getUndo(), m_sprite->getMask(), &m_sprite->getMask()->y);
  }

  m_sprite->getMask()->x = x;
  m_sprite->getMask()->y = y;
}

void Undoable::deselect_mask()
{
  // Destroy the *deselected* mask
  Mask* mask = m_sprite->requestMask("*deselected*");
  if (mask) {
    m_sprite->removeMask(mask);
    mask_free(mask);
  }

  // Save the selection in the repository
  mask = mask_new_copy(m_sprite->getMask());
  mask_set_name(mask, "*deselected*");
  m_sprite->addMask(mask);

  if (undo_is_enabled(m_sprite->getUndo()))
    undo_set_mask(m_sprite->getUndo(), m_sprite);

  /// Deselect the mask
  mask_none(m_sprite->getMask());
}
