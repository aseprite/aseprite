// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/scroll_bar.h"

namespace ui {

void setup_scrollbars(const gfx::Size& scrollableSize,
                      gfx::Rect& viewportArea,
                      Widget& parent,
                      ScrollBar& hbar,
                      ScrollBar& vbar)
{
#define NEED_BAR(w, h, width)                       \
  ((scrollableSize.w > viewportArea.w) &&           \
   (vbar.getBarWidth() < fullViewportArea.w) &&     \
   (hbar.getBarWidth() < fullViewportArea.h))

  const gfx::Rect fullViewportArea = viewportArea;

  hbar.setSize(scrollableSize.w);
  vbar.setSize(scrollableSize.h);

  if (hbar.getParent()) parent.removeChild(&hbar);
  if (vbar.getParent()) parent.removeChild(&vbar);

  if (NEED_BAR(w, h, width)) {
    viewportArea.h -= hbar.getBarWidth();
    parent.addChild(&hbar);

    if (NEED_BAR(h, w, height)) {
      viewportArea.w -= vbar.getBarWidth();
      if (NEED_BAR(w, h, width))
        parent.addChild(&vbar);
      else {
        viewportArea.w += vbar.getBarWidth();
        viewportArea.h += hbar.getBarWidth();
        parent.removeChild(&hbar);
      }
    }
  }
  else if (NEED_BAR(h, w, height)) {
    viewportArea.w -= vbar.getBarWidth();
    parent.addChild(&vbar);

    if (NEED_BAR(w, h, width)) {
      viewportArea.h -= hbar.getBarWidth();
      if (NEED_BAR(h, w, height))
        parent.addChild(&hbar);
      else {
        viewportArea.w += vbar.getBarWidth();
        viewportArea.h += hbar.getBarWidth();
        parent.removeChild(&vbar);
      }
    }
  }

  if (parent.hasChild(&hbar)) {
    hbar.setBounds(gfx::Rect(viewportArea.x, viewportArea.y2(),
                             viewportArea.w, hbar.getBarWidth()));
    hbar.setVisible(true);
  }
  else
    hbar.setVisible(false);

  if (parent.hasChild(&vbar)) {
    vbar.setBounds(gfx::Rect(viewportArea.x2(), viewportArea.y,
                             vbar.getBarWidth(), viewportArea.h));
    vbar.setVisible(true);
  }
  else
    vbar.setVisible(false);
}

} // namespace ui
