// Aseprite Document Library
// Copyright (c) 2019-2023 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/cel.h"

#include "doc/grid.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/sprite.h"
#include "doc/tile.h"
#include "gfx/rect.h"

namespace doc {

Cel::Cel(frame_t frame, const ImageRef& image)
  : Object(ObjectType::Cel)
  , m_layer(NULL)
  , m_frame(frame)
  , m_data(new CelData(image))
{
}

Cel::Cel(frame_t frame, const CelDataRef& celData)
  : Object(ObjectType::Cel)
  , m_layer(NULL)
  , m_frame(frame)
  , m_data(celData)
{
}

// static
Cel* Cel::MakeCopy(const frame_t newFrame,
                   const Cel* other)
{
  Cel* cel = new Cel(newFrame,
                     ImageRef(Image::createCopy(other->image())));

  cel->setPosition(other->position());
  cel->setOpacity(other->opacity());
  cel->setZIndex(other->zIndex());
  return cel;
}

// static
Cel* Cel::MakeLink(const frame_t newFrame,
                   const Cel* other)
{
  Cel* cel = new Cel(newFrame, other->dataRef());
  cel->setZIndex(other->zIndex());
  return cel;
}

void Cel::setFrame(frame_t frame)
{
  ASSERT(m_layer == NULL);
  m_frame = frame;
}

void Cel::setDataRef(const CelDataRef& celData)
{
  ASSERT(celData);
  m_data = celData;
}

void Cel::setPosition(int x, int y)
{
  setPosition(gfx::Point(x, y));
}

void Cel::setPosition(const gfx::Point& pos)
{
  m_data->setPosition(pos);
}

void Cel::setBounds(const gfx::Rect& bounds)
{
  m_data->setBounds(bounds);
}

void Cel::setBoundsF(const gfx::RectF& bounds)
{
  m_data->setBoundsF(bounds);
}

void Cel::setOpacity(int opacity)
{
  m_data->setOpacity(opacity);
}

void Cel::setZIndex(int zindex)
{
  m_zIndex = zindex;
}

Document* Cel::document() const
{
  ASSERT(m_layer);
  if (m_layer && m_layer->sprite())
    return m_layer->sprite()->document();
  else
    return NULL;
}

Sprite* Cel::sprite() const
{
  ASSERT(m_layer);
  if (m_layer)
    return m_layer->sprite();
  else
    return NULL;
}

Cel* Cel::link() const
{
  ASSERT(m_data);
  if (m_data.get() == NULL)
    return NULL;

  if (!m_data.unique()) {
    for (frame_t fr=0; fr<m_frame; ++fr) {
      Cel* possible = m_layer->cel(fr);
      if (possible && possible->dataRef().get() == m_data.get())
        return possible;
    }
  }

  return NULL;
}

std::size_t Cel::links() const
{
  std::size_t links = 0;

  Sprite* sprite = this->sprite();
  for (frame_t fr=0; fr<sprite->totalFrames(); ++fr) {
    Cel* cel = m_layer->cel(fr);
    if (cel && cel != this && cel->dataRef().get() == m_data.get())
      ++links;
  }

  return links;
}

void Cel::setParentLayer(LayerImage* layer)
{
  m_layer = layer;
  fixupImage();
}

Grid Cel::grid() const
{
  if (m_layer) {
    if (m_layer->isTilemap()) {
      doc::Grid grid = static_cast<LayerTilemap*>(m_layer)->tileset()->grid();
      grid.origin(grid.origin() + position());
      return grid;
    }
    else
      return m_layer->grid();
  }
  return Grid();
}

void Cel::fixupImage()
{
  // Change the mask color to the sprite mask color
  if (m_layer && image()) {
    image()->setMaskColor((image()->pixelFormat() == IMAGE_TILEMAP) ?
                            notile : m_layer->sprite()->transparentColor());
    ASSERT(m_data);
    m_data->adjustBounds(m_layer);
  }
}

} // namespace doc
