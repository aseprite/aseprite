// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SKIN_SKIN_PART_H_INCLUDED
#define APP_UI_SKIN_SKIN_PART_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include "gfx/size.h"
#include "os/surface_list.h"

#include <memory>
#include <vector>

namespace os {
class Surface;
}

namespace app { namespace skin {

class SkinPart {
public:
  using Bitmaps = os::SurfaceList;

  SkinPart();
  ~SkinPart();

  std::size_t countBitmaps() const { return m_bitmaps.size(); }
  const gfx::Rect& spriteBounds() const { return m_spriteBounds; }
  const gfx::Rect& slicesBounds() const { return m_slicesBounds; }
  void clear();

  // It doesn't destroy the previous bitmap in the given "index".
  void setBitmap(std::size_t index, const os::SurfaceRef& bitmap);
  void setSpriteBounds(const gfx::Rect& bounds);
  void setSlicesBounds(const gfx::Rect& bounds);

  os::Surface* bitmap(std::size_t index) const
  {
    return (index < m_bitmaps.size() ? const_cast<SkinPart*>(this)->m_bitmaps[index].get() :
                                       nullptr);
  }

  os::SurfaceRef bitmapRef(std::size_t index)
  {
    return (index < m_bitmaps.size() ? const_cast<SkinPart*>(this)->m_bitmaps[index] : nullptr);
  }

  os::Surface* bitmapNW() const { return bitmap(0); }
  os::Surface* bitmapN() const { return bitmap(1); }
  os::Surface* bitmapNE() const { return bitmap(2); }
  os::Surface* bitmapE() const { return bitmap(3); }
  os::Surface* bitmapSE() const { return bitmap(4); }
  os::Surface* bitmapS() const { return bitmap(5); }
  os::Surface* bitmapSW() const { return bitmap(6); }
  os::Surface* bitmapW() const { return bitmap(7); }

  gfx::Size size() const;

private:
  Bitmaps m_bitmaps;
  gfx::Rect m_spriteBounds;
  gfx::Rect m_slicesBounds;
};

typedef std::shared_ptr<SkinPart> SkinPartPtr;

}} // namespace app::skin

#endif
