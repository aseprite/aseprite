// Aseprite
// Copyright (C) 2015  Gabriel Rauter
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_WEBP_OPTIONS_H_INCLUDED
#define APP_FILE_WEBP_OPTIONS_H_INCLUDED
#pragma once

#include "app/file/format_options.h"

#include <webp/decode.h>
#include <webp/encode.h>

namespace app {
  // Data for WebP files
  class WebPOptions : public FormatOptions {
  public:
    WebPOptions(): m_lossless(1), m_quality(75), m_method(6), m_image_hint(WEBP_HINT_DEFAULT), m_image_preset(WEBP_PRESET_DEFAULT) {};

    bool lossless() { return m_lossless; }
    int getQuality() { return m_quality; }
    int getMethod() { return m_method; }
    WebPImageHint getImageHint() { return m_image_hint; }
    WebPPreset getImagePreset() { return m_image_preset; }

    void setLossless(int lossless) { m_lossless = (lossless != 0); }
    void setLossless(bool lossless) { m_lossless = lossless; }
    void setQuality(int quality) { m_quality = quality; }
    void setMethod(int method) { m_method = method; }
    void setImageHint(int imageHint) { m_image_hint = static_cast<WebPImageHint>(imageHint); }
    void setImageHint(WebPImageHint imageHint) { m_image_hint = imageHint; }
    void setImagePreset(int imagePreset) { m_image_preset = static_cast<WebPPreset>(imagePreset); };
    void setImagePreset(WebPPreset imagePreset) { m_image_preset = imagePreset ; }

  private:
    bool m_lossless;           // Lossless encoding (0=lossy(default), 1=lossless).
    int m_quality;          // between 0 (smallest file) and 100 (biggest)
    int m_method;             // quality/speed trade-off (0=fast, 9=slower-better)
    WebPImageHint m_image_hint;  // Hint for image type (lossless only for now).
    WebPPreset m_image_preset;  // Image Preset for lossy webp.
  };

} // namespace app

#endif
