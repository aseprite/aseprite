// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_RECT_H_INCLUDED
#define GUI_RECT_H_INCLUDED

#include "gui/base.h"

namespace ui {

#define jrect_w(r) (((JRect)(r))->x2-((JRect)(r))->x1)
#define jrect_h(r) (((JRect)(r))->y2-((JRect)(r))->y1)

#define jrect_point_in(r,x,y)                                   \
  (((x) >= ((JRect)(r))->x1) && ((x) < ((JRect)(r))->x2) &&     \
   ((y) >= ((JRect)(r))->y1) && ((y) < ((JRect)(r))->y2))

  struct jrect
  {
    int x1, y1, x2, y2;
  };

  JRect jrect_new(int x1, int y1, int x2, int y2);
  JRect jrect_new_copy(const JRect rect);
  void jrect_free(JRect rect);

  void jrect_copy(JRect dst, const JRect src);
  void jrect_replace(JRect rect, int x1, int y1, int x2, int y2);

  void jrect_union(JRect r1, const JRect r2);
  bool jrect_intersect(JRect r1, const JRect r2);

  void jrect_shrink(JRect rect, int border);
  void jrect_stretch(JRect rect, int border);

  void jrect_moveto(JRect rect, int x, int y);
  void jrect_displace(JRect rect, int dx, int dy);

} // namespace ui

#endif
