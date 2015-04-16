// SHE library
// Copyright (C) 2012-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "base/file_handle.h"

#include "SkImageDecoder.h"
#include "SkPixelRef.h"
#include "SkStream.h"

namespace she {

class SkiaSystem : public CommonSystem {
public:
  SkiaSystem()
    : m_defaultDisplay(nullptr) {
  }

  ~SkiaSystem() {
  }

  void dispose() override {
    delete this;
  }

  Capabilities capabilities() const override {
    return Capabilities(
      int(kMultipleDisplaysCapability) |
      int(kCanResizeDisplayCapability) |
      int(kDisplayScaleCapability));
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
    SkAutoTDelete<SkStreamAsset> stream(SkNEW_ARGS(SkFILEStream, (fp.get(), SkFILEStream::kCallerRetains_Ownership)));

    SkAutoTDelete<SkImageDecoder> decoder(SkImageDecoder::Factory(stream));
    // decoder->setRequireUnpremultipliedColors(true);

    if (decoder) {
      stream->rewind();
      SkBitmap bm;
      SkImageDecoder::Result res = decoder->decode(stream, &bm,
        kN32_SkColorType, SkImageDecoder::kDecodePixels_Mode);

      if (res == SkImageDecoder::kSuccess) {
        bm.pixelRef()->setURI(filename);

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

} // namespace she
