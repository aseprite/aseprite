// Aseprite UI Library
// Copyright (C) 2020-2025  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/scroll_window.h"

#include "ui/display.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/window.h"

#include "gfx/rect_io.h"

namespace ui {

View* add_scrollbars(Window* window,
                     const gfx::Rect& workarea,
                     gfx::Rect& bounds,
                     const AddScrollBarsOption option)
{
  gfx::Rect rc = bounds;

  if (rc.x < workarea.x) {
    rc.w -= (workarea.x - rc.x);
    rc.x = workarea.x;
  }
  if (rc.x2() > workarea.x2()) {
    rc.w = workarea.x2() - rc.x;
  }

  bool vscrollbarsAdded = false;
  if (rc.y < workarea.y) {
    rc.h -= (workarea.y - rc.y);
    rc.y = workarea.y;
    vscrollbarsAdded = true;
  }
  if (rc.y2() > workarea.y2()) {
    rc.h = workarea.y2() - rc.y;
    vscrollbarsAdded = true;
  }

  if (option == AddScrollBarsOption::IfNeeded && rc == bounds)
    return nullptr;

  View* view = new View;
  view->InitTheme.connect([view, window] {
    view->noBorderNoChildSpacing();
    view->setStyle(window->theme()->viewBaseStyle());
  });
  view->initTheme();

  if (vscrollbarsAdded) {
    int barWidth = view->verticalBar()->getBarWidth();
    if (get_multiple_displays())
      barWidth *= window->display()->scale();

    rc.w += 2 * barWidth;
    if (rc.x2() > workarea.x2()) {
      rc.x = workarea.x2() - rc.w;
      if (rc.x < workarea.x) {
        rc.x = workarea.x;
        rc.w = workarea.w;
      }
    }
  }

  // New bounds
  bounds = rc;

  Widget* containedWidget = nullptr;
  for (Widget* child : window->children()) {
    if (!child->isDecorative()) {
      containedWidget = child;
    }
  }
  ASSERT(containedWidget);
  if (containedWidget) {
    window->removeChild(containedWidget);
    view->attachToView(containedWidget);
  }
  window->addChild(view);

  return view;
}

} // namespace ui
