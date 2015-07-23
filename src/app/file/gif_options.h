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
    GifOptions(
      bool interlaced = false,
      bool loop = true)
      : m_interlaced(interlaced)
      , m_loop(loop) {
    }

    bool interlaced() const { return m_interlaced; }
    bool loop() const { return m_loop; }

    void setInterlaced(bool interlaced) { m_interlaced = interlaced; }
    void setLoop(bool loop) { m_loop = loop; }

  private:
    bool m_interlaced;
    bool m_loop;
  };

} // namespace app

#endif
