// Aseprite
// Copyright (C) 2018  David Capello
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
    enum Type { Simple, Lossless, Lossy };

    // By default we use 6, because 9 is too slow
    const int kDefaultCompression = 6;

    WebPOptions() : m_loop(true),
                    m_type(Type::Simple),
                    m_compression(kDefaultCompression),
                    m_imageHint(WEBP_HINT_DEFAULT),
                    m_quality(100),
                    m_imagePreset(WEBP_PRESET_DEFAULT) { }

    bool loop() const { return m_loop; }
    Type type() const { return m_type; }
    int compression() const { return m_compression; }
    WebPImageHint imageHint() const { return m_imageHint; }
    int quality() const { return m_quality; }
    WebPPreset imagePreset() const { return m_imagePreset; }

    void setLoop(const bool loop) {
      m_loop = loop;
    }

    void setType(const Type type) {
      m_type = type;

      if (m_type == Type::Simple) {
        m_compression = kDefaultCompression;
        m_imageHint = WEBP_HINT_DEFAULT;
      }
    }

    void setCompression(const int compression) {
      ASSERT(m_type == Type::Lossless);
      m_compression = compression;
    }

    void setImageHint(const WebPImageHint imageHint) {
      ASSERT(m_type == Type::Lossless);
      m_imageHint = imageHint;
    }

    void setQuality(const int quality) {
      ASSERT(m_type == Type::Lossy);
      m_quality = quality;
    }

    void setImagePreset(const WebPPreset imagePreset) {
      ASSERT(m_type == Type::Lossy);
      m_imagePreset = imagePreset;
    }

  private:
    bool m_loop;
    Type m_type;
    // Lossless options
    int m_compression;  // Quality/speed trade-off (0=fast, 9=slower-better)
    WebPImageHint m_imageHint; // Hint for image type (lossless only for now).
    // Lossy options
    int m_quality;      // Between 0 (smallest file) and 100 (biggest)
    WebPPreset m_imagePreset;  // Image Preset for lossy webp.
  };

} // namespace app

#endif
