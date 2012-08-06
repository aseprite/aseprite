// SHE library
// Copyright (C) 2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef SHE_LOCKED_SURFACE_H_INCLUDED
#define SHE_LOCKED_SURFACE_H_INCLUDED

namespace she {

  class LockedSurface {
  public:
    virtual ~LockedSurface() { }
    virtual void unlock() = 0;
    virtual void clear() = 0;
    virtual void blitTo(LockedSurface* dest, int x, int y) const = 0;
  };

} // namespace she

#endif
