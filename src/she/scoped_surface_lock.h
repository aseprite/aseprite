// SHE library
// Copyright (C) 2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef SHE_SCOPED_SURFACE_LOCK_H_INCLUDED
#define SHE_SCOPED_SURFACE_LOCK_H_INCLUDED

#include "she/surface.h"
#include "she/locked_surface.h"

namespace she {

  class ScopedSurfaceLock {
  public:
    ScopedSurfaceLock(Surface* surface) : m_lock(surface->lock()) { }
    ~ScopedSurfaceLock() { m_lock->unlock(); }

    LockedSurface* operator->() { return m_lock; }
    operator LockedSurface*() { return m_lock; }
  private:
    LockedSurface* m_lock;
  };

} // namespace she

#endif
