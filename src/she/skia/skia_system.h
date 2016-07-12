// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_SKIA_SYSTEM_INCLUDED
#define SHE_SKIA_SKIA_SYSTEM_INCLUDED
#pragma once

#include "base/base.h"
#include "base/file_handle.h"

#include "SkCodec.h"
#include "SkPixelRef.h"
#include "SkStream.h"

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
  #define SkiaSystemBase CommonSystem
#else
  #include "she/x11/event_queue.h"
  #define SkiaSystemBase CommonSystem
#endif

namespace she {

EventQueueImpl g_queue;

class SkiaSystem : public SkiaSystemBase {
public:
  SkiaSystem()
    : m_defaultDisplay(nullptr)
    , m_gpuAcceleration(false) {
  }

  ~SkiaSystem() {
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
#if SK_SUPPORT_GPU
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
    base::FileHandle fp(base::open_file_with_exception(filename, "rb"));

    SkAutoTDelete<SkCodec> codec(
      SkCodec::NewFromStream(
        new SkFILEStream(fp.get(), SkFILEStream::kCallerRetains_Ownership)));
    if (!codec)
      return nullptr;

    SkImageInfo info = codec->getInfo()
      .makeColorType(kN32_SkColorType)
      .makeAlphaType(kPremul_SkAlphaType);
    SkBitmap bm;
    if (!bm.tryAllocPixels(info))
      return nullptr;

    const SkCodec::Result r = codec->getPixels(info, bm.getPixels(), bm.rowBytes());
    if (r != SkCodec::kSuccess)
      return nullptr;

    SkiaSurface* sur = new SkiaSurface();
    sur->swapBitmap(bm);
    return sur;
  }

  Surface* loadRgbaSurface(const char* filename) override {
    return loadSurface(filename);
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
