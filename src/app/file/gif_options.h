/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
      DitheringMethod dithering = doc::DITHERING_NONE)
      : m_quantize(quantize)
      , m_interlaced(interlaced)
      , m_dithering(dithering) {
    }

    Quantize quantize() const { return m_quantize; }
    bool interlaced() const { return m_interlaced; }
    doc::DitheringMethod dithering() const { return m_dithering; }

    void setQuantize(const Quantize quantize) { m_quantize = quantize; }
    void setInterlaced(bool interlaced) { m_interlaced = interlaced; }
    void setDithering(const doc::DitheringMethod dithering) { m_dithering = dithering; }

  private:
    Quantize m_quantize;
    bool m_interlaced;
    doc::DitheringMethod m_dithering;
  };

} // namespace app

#endif
