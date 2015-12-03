// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/size.h"
#include "ui/box.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/resize_event.h"
#include "ui/theme.h"

namespace ui {

using namespace gfx;

Box::Box(int align)
  : Widget(kBoxWidget)
{
  setAlign(align);
  initTheme();
}

void Box::onPreferredSize(PreferredSizeEvent& ev)
{
#define ADD_CHILD_SIZE(w, h) {                      \
    if (getAlign() & HOMOGENEOUS)                   \
      prefSize.w = MAX(prefSize.w, childSize.w);    \
    else                                            \
      prefSize.w += childSize.w;                    \
    prefSize.h = MAX(prefSize.h, childSize.h);      \
  }

#define FINAL_ADJUSTMENT(w) {                            \
    if (getAlign() & HOMOGENEOUS)                        \
      prefSize.w *= visibleChildren;                     \
    prefSize.w += childSpacing() * (visibleChildren-1);  \
  }

  int visibleChildren = 0;
  for (auto child : children()) {
    if (!child->hasFlags(HIDDEN))
      ++visibleChildren;
  }

  Size prefSize(0, 0);
  for (auto child : children()) {
    if (child->hasFlags(HIDDEN))
      continue;

    Size childSize = child->getPreferredSize();
    if (getAlign() & HORIZONTAL) {
      ADD_CHILD_SIZE(w, h);
    }
    else {
      ADD_CHILD_SIZE(h, w);
    }
  }

  if (visibleChildren > 0) {
    if (getAlign() & HORIZONTAL) {
      FINAL_ADJUSTMENT(w);
    }
    else {
      FINAL_ADJUSTMENT(h);
    }
  }

  prefSize.w += border().width();
  prefSize.h += border().height();

  ev.setPreferredSize(prefSize);
}

void Box::onResize(ResizeEvent& ev)
{
#define LAYOUT_CHILDREN(x, w) {                                         \
    availExtraSize = availSize.w - prefSize.w;                          \
    availSize.w -= childSpacing() * (visibleChildren-1);                \
    if (getAlign() & HOMOGENEOUS)                                       \
      homogeneousSize = availSize.w / visibleChildren;                  \
                                                                        \
    Rect childPos(getChildrenBounds());                                 \
    int i = 0, j = 0;                                                   \
    for (auto child : children()) {                                     \
      if (child->hasFlags(HIDDEN))                                      \
        continue;                                                       \
                                                                        \
      int size = 0;                                                     \
                                                                        \
      if (getAlign() & HOMOGENEOUS) {                                   \
        if (i < visibleChildren-1)                                      \
          size = homogeneousSize;                                       \
        else                                                            \
          size = availSize.w;                                           \
      }                                                                 \
      else {                                                            \
        size = child->getPreferredSize().w;                             \
                                                                        \
        if (child->isExpansive()) {                                     \
          int extraSize = (availExtraSize / (expansiveChildren-j));     \
          size += extraSize;                                            \
          availExtraSize -= extraSize;                                  \
          if (++j == expansiveChildren)                                 \
            size += availExtraSize;                                     \
        }                                                               \
      }                                                                 \
                                                                        \
      childPos.w = MAX(1, size);                                        \
      child->setBounds(childPos);                                       \
      childPos.x += size + childSpacing();                              \
      availSize.w -= size;                                              \
      ++i;                                                              \
    }                                                                   \
  }

  setBoundsQuietly(ev.getBounds());

  int visibleChildren = 0;
  int expansiveChildren = 0;
  for (auto child : children()) {
    if (!child->hasFlags(HIDDEN)) {
      ++visibleChildren;
      if (child->isExpansive())
        ++expansiveChildren;
    }
  }

  if (visibleChildren > 0) {
    Size prefSize(getPreferredSize());
    Size availSize(getChildrenBounds().getSize());
    int homogeneousSize = 0;
    int availExtraSize = 0;

    prefSize.w -= border().width();
    prefSize.h -= border().height();

    if (getAlign() & HORIZONTAL) {
      LAYOUT_CHILDREN(x, w);
    }
    else {
      LAYOUT_CHILDREN(y, h);
    }
  }
}

void Box::onPaint(PaintEvent& ev)
{
  getTheme()->paintBox(ev);
}

} // namespace ui
