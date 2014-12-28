// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_BLEND_H_INCLUDED
#define DOC_BLEND_H_INCLUDED
#pragma once

#define INT_MULT(a, b, t)                               \
  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

namespace doc {

  enum {
    BLEND_MODE_NORMAL,
    BLEND_MODE_COPY,
    BLEND_MODE_MERGE,
    BLEND_MODE_RED_TINT,
    BLEND_MODE_BLUE_TINT,
    BLEND_MODE_BLACKANDWHITE,
    BLEND_MODE_MAX,
  };

  typedef int (*BLEND_COLOR)(int back, int front, int opacity);

  extern BLEND_COLOR rgba_blenders[];
  extern BLEND_COLOR graya_blenders[];

  int rgba_blend_normal(int back, int front, int opacity);
  int rgba_blend_copy(int back, int front, int opacity);
  int rgba_blend_forpath(int back, int front, int opacity);
  int rgba_blend_merge(int back, int front, int opacity);
  int rgba_blend_red_tint(int back, int front, int opacity);
  int rgba_blend_blue_tint(int back, int front, int opacity);
  int rgba_blend_blackandwhite(int back, int front, int opacity);

  int graya_blend_normal(int back, int front, int opacity);
  int graya_blend_copy(int back, int front, int opacity);
  int graya_blend_forpath(int back, int front, int opacity);
  int graya_blend_merge(int back, int front, int opacity);
  int graya_blend_blackandwhite(int back, int front, int opacity);

  int indexed_blend_direct(int back, int front, int opacity);

} // namespace doc

#endif
