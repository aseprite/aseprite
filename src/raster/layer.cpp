/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "raster/layer.h"

#include "base/unique_ptr.h"
#include "raster/blend.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "raster/stock.h"

#include <algorithm>
#include <string.h>

static bool has_cels(const Layer* layer, int frame);

//////////////////////////////////////////////////////////////////////
// Layer class

Layer::Layer(GfxObjType type, Sprite* sprite)
  : GfxObj(type)
{
  ASSERT(type == GFXOBJ_LAYER_IMAGE || type == GFXOBJ_LAYER_FOLDER);

  setName("Layer");

  m_sprite = sprite;
  m_parent = NULL;
  m_flags =
    LAYER_IS_READABLE |
    LAYER_IS_WRITABLE;
}

Layer::~Layer()
{
}

int Layer::getMemSize() const
{
  return sizeof(Layer);
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
}

LayerImage::~LayerImage()
{
  destroyAllCels();
}

int LayerImage::getMemSize() const
{
  int size = sizeof(LayerImage);
  CelConstIterator it = getCelBegin();
  CelConstIterator end = getCelEnd();

  for (; it != end; ++it) {
    const Cel* cel = *it;
    size += cel->getMemSize();

    const Image* image = getSprite()->getStock()->getImage(cel->getImage());
    size += image->getMemSize();
  }

  return size;
}

void LayerImage::destroyAllCels()
{
  CelIterator it = getCelBegin();
  CelIterator end = getCelEnd();

  for (; it != end; ++it) {
    Cel* cel = *it;
    Image* image = getSprite()->getStock()->getImage(cel->getImage());

    ASSERT(image != NULL);

    getSprite()->getStock()->removeImage(image);
    image_free(image);
    delete cel;
  }
  m_cels.clear();
}

void LayerImage::getCels(CelList& cels)
{
  CelIterator it = getCelBegin();
  CelIterator end = getCelEnd();

  for (; it != end; ++it)
    cels.push_back(*it);
}

void LayerImage::addCel(Cel *cel)
{
  CelIterator it = getCelBegin();
  CelIterator end = getCelEnd();

  for (; it != end; ++it) {
    if ((*it)->getFrame() > cel->getFrame())
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
void LayerImage::removeCel(Cel *cel)
{
  CelIterator it = std::find(m_cels.begin(), m_cels.end(), cel);

  ASSERT(it != m_cels.end());

  m_cels.erase(it);
}

const Cel* LayerImage::getCel(int frame) const
{
  CelConstIterator it = getCelBegin();
  CelConstIterator end = getCelEnd();

  for (; it != end; ++it) {
    const Cel* cel = *it;
    if (cel->getFrame() == frame)
      return cel;
  }

  return NULL;
}

Cel* LayerImage::getCel(int frame)
{
  return const_cast<Cel*>(static_cast<const LayerImage*>(this)->getCel(frame));
}

/**
 * Configures some properties of the specified layer to make it as the
 * "Background" of the sprite.
 *
 * You can't use this routine if the sprite already has a background
 * layer.
 */
void LayerImage::configureAsBackground()
{
  ASSERT(getSprite() != NULL);
  ASSERT(getSprite()->getBackgroundLayer() == NULL);

  set_moveable(false);
  set_background(true);
  setName("Background");

  getSprite()->getFolder()->move_layer(this, NULL);
}

//////////////////////////////////////////////////////////////////////
// LayerFolder class

LayerFolder::LayerFolder(Sprite* sprite)
  : Layer(GFXOBJ_LAYER_FOLDER, sprite)
{
  setName("Layer Set");
}

LayerFolder::~LayerFolder()
{
  destroyAllLayers();
}

void LayerFolder::destroyAllLayers()
{
  LayerIterator it = get_layer_begin();
  LayerIterator end = get_layer_end();

  for (; it != end; ++it) {
    Layer* layer = *it;
    delete layer;
  }
  m_layers.clear();
}

int LayerFolder::getMemSize() const
{
  int size = sizeof(LayerFolder);
  LayerConstIterator it = get_layer_begin();
  LayerConstIterator end = get_layer_end();

  for (; it != end; ++it) {
    const Layer* layer = *it;
    size += layer->getMemSize();
  }

  return size;
}

void LayerFolder::getCels(CelList& cels)
{
  LayerIterator it = get_layer_begin();
  LayerIterator end = get_layer_end();

  for (; it != end; ++it)
    (*it)->getCels(cels);
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
LayerImage* layer_new_flatten_copy(Sprite* dst_sprite, const Layer* src_layer,
                                   int x, int y, int w, int h, int frmin, int frmax)
{
  UniquePtr<LayerImage> flatLayer(new LayerImage(dst_sprite));

  for (int frame=frmin; frame<=frmax; frame++) {
    // Does this frame have cels to render?
    if (has_cels(src_layer, frame)) {
      // Create a new image
      Image* image = Image::create(flatLayer->getSprite()->getImgType(), w, h);

      try {
        // Create the new cel for the output layer (add the image to stock too).
        Cel* cel = new Cel(frame, flatLayer->getSprite()->getStock()->addImage(image));
        cel->setPosition(x, y);

        // Clear the image and render this frame.
        image_clear(image, 0);
        layer_render(src_layer, image, -x, -y, frame);
        flatLayer->addCel(cel);
      }
      catch (...) {
        delete image;
        throw;
      }
    }
  }

  return flatLayer.release();
}

void layer_render(const Layer* layer, Image* image, int x, int y, int frame)
{
  if (!layer->is_readable())
    return;

  switch (layer->getType()) {

    case GFXOBJ_LAYER_IMAGE: {
      const Cel* cel = static_cast<const LayerImage*>(layer)->getCel(frame);
      Image* src_image;

      if (cel) {
        ASSERT((cel->getImage() >= 0) &&
               (cel->getImage() < layer->getSprite()->getStock()->size()));

        src_image = layer->getSprite()->getStock()->getImage(cel->getImage());
        ASSERT(src_image != NULL);

        src_image->mask_color = layer->getSprite()->getTransparentColor();

        image_merge(image, src_image,
                    cel->getX() + x,
                    cel->getY() + y,
                    MID (0, cel->getOpacity(), 255),
                    static_cast<const LayerImage*>(layer)->getBlendMode());
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

  switch (layer->getType()) {

    case GFXOBJ_LAYER_IMAGE:
      return static_cast<const LayerImage*>(layer)->getCel(frame) ? true: false;

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
