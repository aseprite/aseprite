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

#include <algorithm>
#include <string.h>
#include <allegro/unicode.h>

#include "jinete/jlist.h"

#include "raster/blend.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"

static bool has_cels(const Layer* layer, int frame);

//////////////////////////////////////////////////////////////////////
// Layer class

Layer::Layer(int type, Sprite* sprite)
  : GfxObj(type)
{
  ASSERT(type == GFXOBJ_LAYER_IMAGE || type == GFXOBJ_LAYER_FOLDER);

  set_name("Layer");

  m_sprite = sprite;
  m_parent = NULL;
  m_flags =
    LAYER_IS_READABLE |
    LAYER_IS_WRITABLE;
}

Layer::Layer(const Layer* src_layer, Sprite* dst_sprite)
  : GfxObj(src_layer->type)
{
  m_sprite = dst_sprite;
  m_parent = NULL;
  m_flags =
    LAYER_IS_READABLE |
    LAYER_IS_WRITABLE;

  set_name(src_layer->get_name());
  m_flags = src_layer->m_flags;
}

Layer::~Layer()
{
}

/**
 * Gets the previous layer of "layer" that are in the parent set.
 */
Layer* Layer::get_prev() const
{
  if (m_parent != NULL) {
    LayerConstIterator it =
      std::find(m_parent->get_layer_begin(),
		m_parent->get_layer_end(), this);

    if (it != m_parent->get_layer_end() &&
	it != m_parent->get_layer_begin()) {
      it--;
      return *it;
    }
  }
  return NULL;
}

Layer* Layer::get_next() const
{
  if (m_parent != NULL) {
    LayerConstIterator it =
      std::find(m_parent->get_layer_begin(),
		m_parent->get_layer_end(), this);

    if (it != m_parent->get_layer_end()) {
      it++;
      if (it != m_parent->get_layer_end())
	return *it;
    }
  }
  return NULL;
}

//////////////////////////////////////////////////////////////////////
// LayerImage class

LayerImage::LayerImage(Sprite* sprite)
  : Layer(GFXOBJ_LAYER_IMAGE, sprite)
{
  m_blend_mode = BLEND_MODE_NORMAL;
}

LayerImage::LayerImage(const LayerImage* src_layer, Sprite* dst_sprite)
  : Layer(src_layer, dst_sprite)
{
  set_blend_mode(src_layer->get_blend_mode());

  try {
    // copy cels
    CelConstIterator it = src_layer->get_cel_begin();
    CelConstIterator end = src_layer->get_cel_end();

    for (; it != end; ++it) {
      const Cel* cel = *it;
      Cel* cel_copy = cel_new_copy(cel);

      ASSERT((cel->image >= 0) &&
	     (cel->image < src_layer->getSprite()->getStock()->nimage));

      Image* image = src_layer->getSprite()->getStock()->image[cel->image];
      ASSERT(image != NULL);

      Image* image_copy = image_new_copy(image);

      cel_copy->image = stock_add_image(dst_sprite->getStock(), image_copy);
      if (undo_is_enabled(dst_sprite->getUndo()))
	undo_add_image(dst_sprite->getUndo(), dst_sprite->getStock(), cel_copy->image);

      add_cel(cel_copy);
    }
  }
  catch (...) {
    destroy_all_cels();
    throw;
  }
}

LayerImage::~LayerImage()
{
  destroy_all_cels();
}

void LayerImage::destroy_all_cels()
{
  CelIterator it = get_cel_begin();
  CelIterator end = get_cel_end();

  for (; it != end; ++it) {
    Cel* cel = *it;
    Image* image = getSprite()->getStock()->image[cel->image];

    ASSERT(image != NULL);

    stock_remove_image(getSprite()->getStock(), image);
    image_free(image);
    cel_free(cel);
  }
  m_cels.clear();
}

void LayerImage::set_blend_mode(int blend_mode)
{
  m_blend_mode = blend_mode;
}

void LayerImage::get_cels(CelList& cels)
{
  CelIterator it = get_cel_begin();
  CelIterator end = get_cel_end();

  for (; it != end; ++it)
    cels.push_back(*it);
}

void LayerImage::add_cel(Cel *cel)
{
  CelIterator it = get_cel_begin();
  CelIterator end = get_cel_end();

  for (; it != end; ++it) {
    if ((*it)->frame > cel->frame)
      break;
  }

  m_cels.insert(it, cel);
}

/**
 * Removes the cel from the layer.
 *
 * It doesn't destroy the cel, you have to delete it after calling
 * this routine.
 */
void LayerImage::remove_cel(Cel *cel)
{
  CelIterator it = std::find(m_cels.begin(), m_cels.end(), cel);

  ASSERT(it != m_cels.end());

  m_cels.erase(it);
}

const Cel* LayerImage::get_cel(int frame) const
{
  CelConstIterator it = get_cel_begin();
  CelConstIterator end = get_cel_end();

  for (; it != end; ++it) {
    const Cel* cel = *it;
    if (cel->frame == frame)
      return cel;
  }

  return NULL;
}

Cel* LayerImage::get_cel(int frame)
{
  CelIterator it = get_cel_begin();
  CelIterator end = get_cel_end();

  for (; it != end; ++it) {
    Cel* cel = *it;
    if (cel->frame == frame)
      return cel;
  }

  return NULL;
}

/**
 * Configures some properties of the specified layer to make it as the
 * "Background" of the sprite.
 *
 * You can't use this routine if the sprite already has a background
 * layer.
 */
void LayerImage::configure_as_background()
{
  ASSERT(getSprite() != NULL);
  ASSERT(getSprite()->getBackgroundLayer() == NULL);

  *flags_addr() |= LAYER_IS_LOCKMOVE | LAYER_IS_BACKGROUND;
  set_name("Background");

  getSprite()->getFolder()->move_layer(this, NULL);
}

//////////////////////////////////////////////////////////////////////
// LayerFolder class

LayerFolder::LayerFolder(Sprite* sprite)
  : Layer(GFXOBJ_LAYER_FOLDER, sprite)
{
  set_name("Layer Set");
}

LayerFolder::LayerFolder(const LayerFolder* src_layer, Sprite* dst_sprite)
  : Layer(src_layer, dst_sprite)
{
  try {
    LayerConstIterator it = src_layer->get_layer_begin();
    LayerConstIterator end = src_layer->get_layer_end();

    for (; it != end; ++it) {
      // duplicate the child
      Layer* child_copy = (*it)->duplicate_for(dst_sprite);

      // add the new child in the layer copy
      add_layer(child_copy);
    }
  }
  catch (...) {
    destroy_all_layers();
    throw;
  }
}

LayerFolder::~LayerFolder()
{
  destroy_all_layers();
}

void LayerFolder::destroy_all_layers()
{
  LayerIterator it = get_layer_begin();
  LayerIterator end = get_layer_end();

  for (; it != end; ++it) {
    Layer* layer = *it;
    delete layer;
  }
  m_layers.clear();
}

void LayerFolder::get_cels(CelList& cels)
{
  LayerIterator it = get_layer_begin();
  LayerIterator end = get_layer_end();

  for (; it != end; ++it)
    (*it)->get_cels(cels);
}

void LayerFolder::add_layer(Layer* layer)
{
  m_layers.push_back(layer);
  layer->set_parent(this);
}

void LayerFolder::remove_layer(Layer* layer)
{
  LayerIterator it = std::find(m_layers.begin(), m_layers.end(), layer);
  ASSERT(it != m_layers.end());
  m_layers.erase(it);

  layer->set_parent(NULL);
}

void LayerFolder::move_layer(Layer* layer, Layer* after)
{
  LayerIterator it = std::find(m_layers.begin(), m_layers.end(), layer);
  ASSERT(it != m_layers.end());
  m_layers.erase(it);

  if (after) {
    LayerIterator after_it = std::find(m_layers.begin(), m_layers.end(), after);
    ASSERT(after_it != m_layers.end());
    after_it++;
    m_layers.insert(after_it, layer);
  }
  else
    m_layers.push_front(layer);

  // TODO
  // if (after) {
  //   JLink before = jlist_find(m_layers, after)->next;
  //   jlist_insert_before(m_layers, before, layer);
  // }
  // else
  //   jlist_prepend(m_layers, layer);
}

//////////////////////////////////////////////////////////////////////

/**
 * Returns a new layer (flat_layer) with all "layer" rendered frame by
 * frame from "frmin" to "frmax" (inclusive).  "layer" can be a set of
 * layers, so the routine flattens all children to an unique output
 * layer.
 *
 * @param dst_sprite The sprite where to put the new flattened layer.
 * @param src_layer Generally a set of layers to be flattened.
 */
Layer* layer_new_flatten_copy(Sprite* dst_sprite, const Layer* src_layer,
			      int x, int y, int w, int h, int frmin, int frmax)
{
  LayerImage* flat_layer = new LayerImage(dst_sprite);

  try {
    for (int frame=frmin; frame<=frmax; frame++) {
      /* does this frame have cels to render? */
      if (has_cels(src_layer, frame)) {
	/* create a new image */
	Image* image = image_new(flat_layer->getSprite()->getImgType(), w, h);

	try {
	  /* create the new cel for the output layer (add the image to
	     stock too) */
	  Cel* cel = cel_new(frame, stock_add_image(flat_layer->getSprite()->getStock(), image));
	  cel_set_position(cel, x, y);

	  /* clear the image and render this frame */
	  image_clear(image, 0);
	  layer_render(src_layer, image, -x, -y, frame);
	  flat_layer->add_cel(cel);
	}
	catch (...) {
	  delete image;
	  throw;
	}
      }
    }
  }
  catch (...) {
    delete flat_layer;
    throw;
  }

  return flat_layer;
}

void layer_render(const Layer* layer, Image* image, int x, int y, int frame)
{
  if (!layer->is_readable())
    return;

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE: {
      const Cel* cel = static_cast<const LayerImage*>(layer)->get_cel(frame);
      Image* src_image;

      if (cel) {
	ASSERT((cel->image >= 0) &&
	       (cel->image < layer->getSprite()->getStock()->nimage));

	src_image = layer->getSprite()->getStock()->image[cel->image];
	ASSERT(src_image != NULL);

	image_merge(image, src_image,
		    cel->x + x,
		    cel->y + y,
		    MID (0, cel->opacity, 255),
		    static_cast<const LayerImage*>(layer)->get_blend_mode());
      }
      break;
    }

    case GFXOBJ_LAYER_FOLDER: {
      LayerConstIterator it = static_cast<const LayerFolder*>(layer)->get_layer_begin();
      LayerConstIterator end = static_cast<const LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it)
	layer_render(*it, image, x, y, frame);

      break;
    }

  }
}

/**
 * Returns true if the "layer" (or him childs) has cels to render in
 * frame.
 */
static bool has_cels(const Layer* layer, int frame)
{
  if (!layer->is_readable())
    return false;

  switch (layer->type) {

    case GFXOBJ_LAYER_IMAGE:
      return static_cast<const LayerImage*>(layer)->get_cel(frame) ? true: false;

    case GFXOBJ_LAYER_FOLDER: {
      LayerConstIterator it = static_cast<const LayerFolder*>(layer)->get_layer_begin();
      LayerConstIterator end = static_cast<const LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it) {
	if (has_cels(*it, frame))
	  return true;
      }
      break;
    }

  }

  return false;
}
