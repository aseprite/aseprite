// Aseprite UI Library
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/clamp.h"
#include "gfx/size.h"
#include "ui/box.h"
#include "ui/message.h"
#include "ui/resize_event.h"
#include "ui/size_hint_event.h"
#include "ui/theme.h"

#include <algorithm>

namespace ui {

using namespace gfx;

Box::Box(int align)
  : Widget(kBoxWidget)
{
  enableFlags(IGNORE_MOUSE);
  setAlign(align);
  initTheme();
}

void Box::onSizeHint(SizeHintEvent& ev)
{
#define ADD_CHILD_SIZE(w, h) {                                 \
    if (align() & HOMOGENEOUS)                                 \
      prefSize.w = std::max(prefSize.w, childSize.w);          \
    else                                                       \
      prefSize.w += childSize.w;                               \
    prefSize.h = std::max(prefSize.h, childSize.h);            \
  }

#define FINAL_ADJUSTMENT(w) {                            \
    if (align() & HOMOGENEOUS)                           \
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

    Size childSize = child->sizeHint();
    if (align() & HORIZONTAL) {
      ADD_CHILD_SIZE(w, h);
    }
    else {
      ADD_CHILD_SIZE(h, w);
    }
  }

  if (visibleChildren > 0) {
    if (align() & HORIZONTAL) {
      FINAL_ADJUSTMENT(w);
    }
    else {
      FINAL_ADJUSTMENT(h);
    }
  }

  prefSize.w += border().width();
  prefSize.h += border().height();

  ev.setSizeHint(prefSize);
}

void Box::onResize(ResizeEvent& ev)
{
#define LAYOUT_CHILDREN(x, y, w, h) {                                   \
    availExtraSize = availSize.w - prefSize.w;                          \
    availSize.w -= childSpacing() * (visibleChildren-1);                \
    if (align() & HOMOGENEOUS)                                          \
      homogeneousSize = availSize.w / visibleChildren;                  \
                                                                        \
    Rect defChildPos(childrenBounds());                                 \
    int i = 0, j = 0;                                                   \
    for (auto child : children()) {                                     \
      if (child->hasFlags(HIDDEN))                                      \
        continue;                                                       \
                                                                        \
      int size = 0;                                                     \
                                                                        \
      if (align() & HOMOGENEOUS) {                                      \
        if (i < visibleChildren-1)                                      \
          size = homogeneousSize;                                       \
        else                                                            \
          size = availSize.w;                                           \
      }                                                                 \
      else {                                                            \
        size = child->sizeHint().w;                                     \
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
      Rect childPos = defChildPos;                                      \
      childPos.w = size = base::clamp(size, child->minSize().w, child->maxSize().w); \
      childPos.h = base::clamp(childPos.h, child->minSize().h, child->maxSize().h); \
      child->setBounds(childPos);                                       \
                                                                        \
      defChildPos.x += size + childSpacing();                           \
      availSize.w -= size;                                              \
      ++i;                                                              \
    }                                                                   \
  }

  setBoundsQuietly(ev.bounds());

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
    Size prefSize(sizeHint());
    Size availSize(childrenBounds().size());
    int homogeneousSize = 0;
    int availExtraSize = 0;

    prefSize.w -= border().width();
    prefSize.h -= border().height();

    if (align() & HORIZONTAL) {
      LAYOUT_CHILDREN(x, y, w, h);
    }
    else {
      LAYOUT_CHILDREN(y, x, h, w);
    }
  }
}

} // namespace ui
