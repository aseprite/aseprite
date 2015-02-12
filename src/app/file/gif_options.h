// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_FILE_GIF_OPTIONS_H_INCLUDED
#define APP_FILE_GIF_OPTIONS_H_INCLUDED
#pragma once

#include "app/file/format_options.h"
#include "doc/dithering_method.h"

namespace app {

  // Data for GIF files
  class GifOptions : public FormatOptions {
  public:
    enum Quantize { NoQuantize, QuantizeEach, QuantizeAll };

    GifOptions(
      Quantize quantize = QuantizeEach,
      bool interlaced = false,
      bool loop = true,
      DitheringMethod dithering = doc::DitheringMethod::NONE)
      : m_quantize(quantize)
      , m_interlaced(interlaced)
      , m_loop(loop)
      , m_dithering(dithering) {
    }

    Quantize quantize() const { return m_quantize; }
    bool interlaced() const { return m_interlaced; }
    bool loop() const { return m_loop; }
    doc::DitheringMethod dithering() const { return m_dithering; }

    void setQuantize(const Quantize quantize) { m_quantize = quantize; }
    void setInterlaced(bool interlaced) { m_interlaced = interlaced; }
    void setLoop(bool loop) { m_loop = loop; }
    void setDithering(const doc::DitheringMethod dithering) { m_dithering = dithering; }

  private:
    Quantize m_quantize;
    bool m_interlaced;
    bool m_loop;
    doc::DitheringMethod m_dithering;
  };

} // namespace app

#endif
