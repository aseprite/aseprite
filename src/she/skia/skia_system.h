// SHE library
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_SKIA_SYSTEM_INCLUDED
#define SHE_SKIA_SKIA_SYSTEM_INCLUDED
#pragma once

#include "base/base.h"

#include "she/common/system.h"
#include "she/skia/skia_display.h"
#include "she/skia/skia_surface.h"

#ifdef _WIN32
  #include "she/win/event_queue.h"
  #include "she/win/system.h"
  #define SkiaSystemBase WindowSystem
#elif __APPLE__
  #include "she/osx/app.h"
  #include "she/osx/event_queue.h"
  #include "she/osx/system.h"
  #define SkiaSystemBase OSXSystem
#else
  #include "she/x11/event_queue.h"
  #include "she/x11/system.h"
  #define SkiaSystemBase X11System
#endif

#include "SkGraphics.h"

namespace she {

EventQueueImpl g_queue;

class SkiaSystem : public SkiaSystemBase {
public:
  SkiaSystem()
    : m_defaultDisplay(nullptr)
    , m_gpuAcceleration(false) {
    SkGraphics::Init();
  }

  ~SkiaSystem() {
    SkGraphics::Term();
  }

  void dispose() override {
    delete this;
  }

  void activateApp() override {
#if __APPLE__
    OSXApp::instance()->activateApp();
#endif
  }

  void finishLaunching() override {
#if __APPLE__
    // Start processing NSApplicationDelegate events. (E.g. after
    // calling this we'll receive application:openFiles: and we'll
    // generate DropFiles events.)  events
    OSXApp::instance()->finishLaunching();
#endif
  }

  Capabilities capabilities() const override {
    return Capabilities(
      int(Capabilities::MultipleDisplays) |
      int(Capabilities::CanResizeDisplay) |
      int(Capabilities::DisplayScale)
#if defined(_WIN32) || defined(__APPLE__)
      | int(Capabilities::CustomNativeMouseCursor)
#endif
    // TODO enable this when the GPU support is ready
#if 0 // SK_SUPPORT_GPU
      | int(Capabilities::GpuAccelerationSwitch)
#endif
      );
  }

  EventQueue* eventQueue() override {
    return &g_queue;
  }

  bool gpuAcceleration() const override {
    return m_gpuAcceleration;
  }

  void setGpuAcceleration(bool state) override {
    m_gpuAcceleration = state;
  }

  gfx::Size defaultNewDisplaySize() override {
    gfx::Size sz;
#ifdef _WIN32
    sz.w = GetSystemMetrics(SM_CXMAXIMIZED);
    sz.h = GetSystemMetrics(SM_CYMAXIMIZED);
    sz.w -= GetSystemMetrics(SM_CXSIZEFRAME)*4;
    sz.h -= GetSystemMetrics(SM_CYSIZEFRAME)*4;
    sz.w = MAX(0, sz.w);
    sz.h = MAX(0, sz.h);
#endif
    return sz;
  }

  Display* defaultDisplay() override {
    return m_defaultDisplay;
  }

  Display* createDisplay(int width, int height, int scale) override {
    SkiaDisplay* display = new SkiaDisplay(width, height, scale);
    if (!m_defaultDisplay)
      m_defaultDisplay = display;
    return display;
  }

  Surface* createSurface(int width, int height) override {
    SkiaSurface* sur = new SkiaSurface;
    sur->create(width, height);
    return sur;
  }

  Surface* createRgbaSurface(int width, int height) override {
    SkiaSurface* sur = new SkiaSurface;
    sur->createRgba(width, height);
    return sur;
  }

  Surface* loadSurface(const char* filename) override {
    return SkiaSurface::loadSurface(filename);
  }

  Surface* loadRgbaSurface(const char* filename) override {
    return loadSurface(filename);
  }

  void setTranslateDeadKeys(bool state) override {
    if (m_defaultDisplay)
      m_defaultDisplay->setTranslateDeadKeys(state);

#ifdef _WIN32
    g_queue.setTranslateDeadKeys(state);
#endif
  }

private:
  SkiaDisplay* m_defaultDisplay;
  bool m_gpuAcceleration;
};

EventQueue* EventQueue::instance() {
  return &g_queue;
}

} // namespace she

#endif
