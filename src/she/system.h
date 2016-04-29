// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SYSTEM_H_INCLUDED
#define SHE_SYSTEM_H_INCLUDED
#pragma once

#include "gfx/fwd.h"
#include "she/capabilities.h"

#include <stdexcept>

namespace she {

  class Display;
  class EventQueue;
  class Font;
  class Logger;
  class NativeDialogs;
  class Surface;

  class DisplayCreationException : public std::runtime_error {
  public:
    DisplayCreationException(const char* msg) throw()
      : std::runtime_error(msg) { }
  };

  class System {
  public:
    virtual ~System() { }
    virtual void dispose() = 0;
    virtual void finishLaunching() = 0;
    virtual Capabilities capabilities() const = 0;
    virtual Logger* logger() = 0;
    virtual NativeDialogs* nativeDialogs() = 0;
    virtual EventQueue* eventQueue() = 0;
    virtual bool gpuAcceleration() const = 0;
    virtual void setGpuAcceleration(bool state) = 0;
    virtual gfx::Size defaultNewDisplaySize() = 0;
    virtual Display* defaultDisplay() = 0;
    virtual Display* createDisplay(int width, int height, int scale) = 0;
    virtual Surface* createSurface(int width, int height) = 0;
    virtual Surface* createRgbaSurface(int width, int height) = 0;
    virtual Surface* loadSurface(const char* filename) = 0;
    virtual Surface* loadRgbaSurface(const char* filename) = 0;
    virtual Font* loadSpriteSheetFont(const char* filename, int scale = 1) = 0;
    virtual Font* loadTrueTypeFont(const char* filename, int height) = 0;
  };

  System* create_system();
  System* instance();

} // namespace she

#endif
