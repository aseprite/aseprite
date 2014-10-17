// SHE library
// Copyright (C) 2012-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SURFACE_H_INCLUDED
#define SHE_SURFACE_H_INCLUDED
#pragma once

#include "gfx/fwd.h"

namespace she {

  class LockedSurface;

  enum class DrawMode {
    Solid,
    Checked
  };

  class Surface {
  public:
    virtual ~Surface() { }
    virtual void dispose() = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual bool isDirectToScreen() const = 0;

    virtual gfx::Rect getClipBounds() = 0;
    virtual void setClipBounds(const gfx::Rect& rc) = 0;
    virtual bool intersectClipRect(const gfx::Rect& rc) = 0;

    virtual void setDrawMode(DrawMode mode, int param = 0) = 0;

    virtual LockedSurface* lock() = 0;
    virtual void applyScale(int scaleFactor) = 0;
    virtual void* nativeHandle() = 0;
  };

  class NonDisposableSurface : public Surface {
  public:
    virtual ~NonDisposableSurface() { }
  private:
    virtual void dispose() = 0;
  };

} // namespace she

#endif
