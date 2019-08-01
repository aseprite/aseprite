// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
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

    void setImage(const ImageRef& image);
    void setPosition(const gfx::Point& pos) { m_bounds.setOrigin(pos); }
    void setOpacity(int opacity) { m_opacity = opacity; }

    void setBounds(const gfx::Rect& bounds) {
      m_bounds = bounds;
      if (m_boundsF)
        *m_boundsF = gfx::RectF(bounds);
    }

    void setBoundsF(const gfx::RectF& boundsF) {
      if (m_boundsF)
        *m_boundsF = boundsF;
      else
        m_boundsF = new gfx::RectF(boundsF);

      m_bounds = gfx::Rect(boundsF);
    }

    const gfx::RectF& boundsF() const {
      if (!m_boundsF)
        m_boundsF = new gfx::RectF(m_bounds);
      return *m_boundsF;
    }

    virtual int getMemSize() const override {
      ASSERT(m_image);
      return sizeof(CelData) + m_image->getMemSize();
    }

  private:
    ImageRef m_image;
    int m_opacity;
    gfx::Rect m_bounds;

    // Special bounds for reference layers that can have subpixel
    // position.
    mutable gfx::RectF* m_boundsF;
  };

  typedef std::shared_ptr<CelData> CelDataRef;

} // namespace doc

#endif
