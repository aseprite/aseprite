// Aseprite Document Library
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CEL_DATA_H_INCLUDED
#define DOC_CEL_DATA_H_INCLUDED
#pragma once

#include "doc/image_ref.h"
#include "doc/object.h"
#include "doc/with_user_data.h"
#include "gfx/rect.h"

#include <memory>

namespace doc {

class Layer;
class Tileset;

class CelData : public WithUserData {
public:
  CelData(const ImageRef& image);
  CelData(const CelData& celData);
  ~CelData();

  gfx::Point position() const { return m_bounds.origin(); }
  const gfx::Rect& bounds() const { return m_bounds; }
  int opacity() const { return m_opacity; }
  Image* image() const { return const_cast<Image*>(m_image.get()); };
  ImageRef imageRef() const { return m_image; }

  // Returns a rectangle with the bounds of the image (width/height
  // of the image) in the position of the cel (useful to compare
  // active tilemap bounds when we have to change the tilemap cel
  // bounds).
  gfx::Rect imageBounds() const
  {
    return gfx::Rect(m_bounds.x, m_bounds.y, m_image->width(), m_image->height());
  }

  void setImage(const ImageRef& image, Layer* layer);
  void setPosition(const gfx::Point& pos);

  void setOpacity(int opacity) { m_opacity = opacity; }

  void setBounds(const gfx::Rect& bounds)
  {
    ASSERT(bounds.w > 0);
    ASSERT(bounds.h > 0);
    m_bounds = bounds;
    if (m_boundsF)
      *m_boundsF = gfx::RectF(bounds);
  }

  void setBoundsF(const gfx::RectF& boundsF)
  {
    if (m_boundsF)
      *m_boundsF = boundsF;
    else
      m_boundsF = std::make_unique<gfx::RectF>(boundsF);

    m_bounds = gfx::Rect(boundsF);
    if (m_bounds.w <= 0)
      m_bounds.w = 1;
    if (m_bounds.h <= 0)
      m_bounds.h = 1;
  }

  const gfx::RectF& boundsF() const
  {
    if (!m_boundsF)
      m_boundsF = std::make_unique<gfx::RectF>(m_bounds);
    return *m_boundsF;
  }

  bool hasBoundsF() const { return m_boundsF != nullptr; }

  virtual int getMemSize() const override
  {
    ASSERT(m_image);
    return sizeof(CelData) + m_image->getMemSize();
  }

  void adjustBounds(Layer* layer);

private:
  ImageRef m_image;
  int m_opacity;
  gfx::Rect m_bounds;

  // Special bounds for reference layers that can have subpixel
  // position.
  mutable std::unique_ptr<gfx::RectF> m_boundsF;
};

typedef std::shared_ptr<CelData> CelDataRef;

} // namespace doc

#endif
