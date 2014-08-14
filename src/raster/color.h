/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifndef RASTER_COLOR_H_INCLUDED
#define RASTER_COLOR_H_INCLUDED
#pragma once

namespace raster {

  // The greatest int type to storage a color for an image in the
  // available pixel formats.
  typedef uint32_t color_t;

  //////////////////////////////////////////////////////////////////////
  // RGBA

  const uint32_t rgba_r_shift = 0;
  const uint32_t rgba_g_shift = 8;
  const uint32_t rgba_b_shift = 16;
  const uint32_t rgba_a_shift = 24;

  const uint32_t rgba_r_mask = 0x000000ff;
  const uint32_t rgba_g_mask = 0x0000ff00;
  const uint32_t rgba_b_mask = 0x00ff0000;
  const uint32_t rgba_rgb_mask = 0x00ffffff;
  const uint32_t rgba_a_mask = 0xff000000;

  inline uint8_t rgba_getr(uint32_t c) {
    return (c >> rgba_r_shift) & 0xff;
  }

  inline uint8_t rgba_getg(uint32_t c) {
    return (c >> rgba_g_shift) & 0xff;
  }

  inline uint8_t rgba_getb(uint32_t c) {
    return (c >> rgba_b_shift) & 0xff;
  }

  inline uint8_t rgba_geta(uint32_t c) {
    return (c >> rgba_a_shift) & 0xff;
  }

  inline uint32_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return ((r << rgba_r_shift) |
            (g << rgba_g_shift) |
            (b << rgba_b_shift) |
            (a << rgba_a_shift));
  }

  inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return rgba(0, 0, 0, 255);
  }

  //////////////////////////////////////////////////////////////////////
  // Grayscale

  const uint16_t graya_v_shift = 0;
  const uint16_t graya_a_shift = 8;

  const uint16_t graya_v_mask = 0x00ff;
  const uint16_t graya_a_mask = 0xff00;

  inline uint8_t graya_getv(uint16_t c) {
    return (c >> graya_v_shift) & 0xff;
  }

  inline uint8_t graya_geta(uint16_t c) {
    return (c >> graya_a_shift) & 0xff;
  }

  inline uint16_t graya(uint8_t v, uint8_t a) {
    return ((v << graya_v_shift) |
            (a << graya_a_shift));
  }

  inline uint16_t gray(uint8_t v) {
    return graya(v, 255);
  }

} // namespace raster

#endif
