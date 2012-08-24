// SHE library
// Copyright (C) 2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef SHE_SYSTEM_H_INCLUDED
#define SHE_SYSTEM_H_INCLUDED

#include "she/capabilities.h"
#include <stdexcept>

namespace she {

  class Surface;
  class Display;

  class DisplayCreationException : std::runtime_error {
  public:
    DisplayCreationException(const char* msg) throw()
      : std::runtime_error(msg) { }
  };

  class System {
  public:
    virtual ~System() { }
    virtual void dispose() = 0;
    virtual Capabilities capabilities() const = 0;
    virtual Display* createDisplay(int width, int height, int scale) = 0;
    virtual Surface* createSurface(int width, int height) = 0;
    virtual Surface* createSurfaceFromNativeHandle(void* nativeHandle) = 0;
  };

  System* CreateSystem();
  System* Instance();

} // namespace she

#endif
