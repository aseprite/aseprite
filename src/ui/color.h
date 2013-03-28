// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_COLOR_H_INCLUDED
#define UI_COLOR_H_INCLUDED

namespace ui {

  typedef uint32_t Color;
  typedef uint8_t ColorComponent;

  static const int ColorRShift = 0;
  static const int ColorGShift = 8;
  static const int ColorBShift = 16;
  static const int ColorAShift = 24;

  static const Color ColorNone = Color(0);

  inline Color rgba(ColorComponent r, ColorComponent g, ColorComponent b, ColorComponent a = 255) {
    return Color((r << ColorRShift) |
                 (g << ColorGShift) |
                 (b << ColorBShift) |
                 (a << ColorAShift));
  }

  inline ColorComponent getr(Color c) { return (c >> ColorRShift) & 0xff; }
  inline ColorComponent getg(Color c) { return (c >> ColorGShift) & 0xff; }
  inline ColorComponent getb(Color c) { return (c >> ColorBShift) & 0xff; }
  inline ColorComponent geta(Color c) { return (c >> ColorAShift) & 0xff; }

  inline Color setr(Color c, ColorComponent v) { return Color((c & ~ColorRShift) | (v << ColorRShift)); }
  inline Color setg(Color c, ColorComponent v) { return Color((c & ~ColorGShift) | (v << ColorGShift)); }
  inline Color setb(Color c, ColorComponent v) { return Color((c & ~ColorBShift) | (v << ColorBShift)); }
  inline Color seta(Color c, ColorComponent v) { return Color((c & ~ColorAShift) | (v << ColorAShift)); }

  inline bool is_transparent(Color c) { return geta(c) == 0; }

  int to_system(Color color);
  
} // namespace ui

#endif  // UI_COLOR_H_INCLUDED
