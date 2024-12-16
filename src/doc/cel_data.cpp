// Aseprite Document Library
// Copyright (c) 2019-2022 Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/cel_data.h"

#include "doc/image.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/sprite.h"
#include "doc/tileset.h"
#include "gfx/rect.h"

namespace doc {

CelData::CelData(const ImageRef& image)
  : WithUserData(ObjectType::CelData)
  , m_image(image)
  , m_opacity(255)
  , m_bounds(0, 0, image ? image->width() : 0, image ? image->height() : 0)
  , m_boundsF(nullptr)
{
}

CelData::CelData(const CelData& celData)
  : WithUserData(ObjectType::CelData)
  , m_image(celData.m_image)
  , m_opacity(celData.m_opacity)
  , m_bounds(celData.m_bounds)
  , m_boundsF(celData.m_boundsF ? std::make_unique<gfx::RectF>(*celData.m_boundsF) : nullptr)
{
}

CelData::~CelData()
{
}

void CelData::setImage(const ImageRef& image, Layer* layer)
{
  ASSERT(image.get());

  m_image = image;
  adjustBounds(layer);
}

void CelData::setPosition(const gfx::Point& pos)
{
  m_bounds.setOrigin(pos);
  if (m_boundsF)
    m_boundsF->setOrigin(gfx::PointF(pos));
}

void CelData::adjustBounds(Layer* layer)
{
  ASSERT(m_image);
  if (m_image->pixelFormat() == IMAGE_TILEMAP) {
    Tileset* tileset = nullptr;
    if (layer && layer->isTilemap())
      tileset = static_cast<LayerTilemap*>(layer)->tileset();
    if (tileset) {
      gfx::Size canvasSize = tileset->grid().tilemapSizeToCanvas(
        gfx::Size(m_image->width(), m_image->height()));
      m_bounds.w = canvasSize.w;
      m_bounds.h = canvasSize.h;
      return;
    }
  }
  m_bounds.w = m_image->width();
  m_bounds.h = m_image->height();
}

} // namespace doc
