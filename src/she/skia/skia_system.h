// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_SKIA_SKIA_SYSTEM_INCLUDED
#define SHE_SKIA_SKIA_SYSTEM_INCLUDED
#pragma once

#include "base/file_handle.h"

#include "SkImageDecoder.h"
#include "SkPixelRef.h"
#include "SkStream.h"

#include "she/skia/skia_display.h"
#include "she/skia/skia_surface.h"

#ifdef _WIN32
  #include "she/win/event_queue.h"
#elif __APPLE__
  #include "she/osx/app.h"
  #include "she/osx/event_queue.h"
#else
  #error There is no EventQueue implementation for your platform
#endif

namespace she {

EventQueueImpl g_queue;

class SkiaSystem : public CommonSystem {
public:
  SkiaSystem()
    : m_defaultDisplay(nullptr) {
  }

  ~SkiaSystem() {
#if __APPLE__
    OSXApp::instance()->stopUIEventLoop();
#endif
  }

  void dispose() override {
    delete this;
  }

  Capabilities capabilities() const override {
    return Capabilities(
      int(Capabilities::MultipleDisplays) |
      int(Capabilities::CanResizeDisplay) |
      int(Capabilities::DisplayScale));
  }

  EventQueue* eventQueue() override {
    return &g_queue;
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
    SkAutoTDelete<SkStreamAsset> stream(new SkFILEStream(fp.get(), SkFILEStream::kCallerRetains_Ownership));

    SkAutoTDelete<SkImageDecoder> decoder(SkImageDecoder::Factory(stream));
    if (decoder) {
      stream->rewind();
      SkBitmap bm;
      SkImageDecoder::Result res = decoder->decode(stream, &bm,
        kN32_SkColorType, SkImageDecoder::kDecodePixels_Mode);

      if (res == SkImageDecoder::kSuccess) {
        // TODO Check why this line crashes on OS X
        //bm.pixelRef()->setURI(filename);

        SkiaSurface* sur = new SkiaSurface();
        sur->swapBitmap(bm);
        return sur;
      }
    }
    return nullptr;
  }

  Surface* loadRgbaSurface(const char* filename) override {
    return loadSurface(filename);
  }

private:
  SkiaDisplay* m_defaultDisplay;
};

EventQueue* EventQueue::instance() {
  return &g_queue;
}

} // namespace she

#endif
