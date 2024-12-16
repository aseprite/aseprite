// Aseprite UI Library
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/fit_bounds.h"

#include "gfx/rect.h"
#include "os/screen.h"
#include "os/system.h"
#include "ui/base.h"
#include "ui/display.h"
#include "ui/system.h"
#include "ui/window.h"

#include <algorithm>

namespace ui {

#if 0 // TODO unused function, referenced in a comment in this file
static gfx::Region get_workarea_region()
{
  // Returns the union of the workarea of all available screens
  gfx::Region wa;
  os::ScreenList screens;
  os::instance()->listScreens(screens);
  for (const auto& screen : screens)
    wa |= gfx::Region(screen->workarea());
  return wa;
}
#endif

int fit_bounds(Display* display, int arrowAlign, const gfx::Rect& target, gfx::Rect& bounds)
{
  bounds.x = target.x;
  bounds.y = target.y;

  int trycount = 0;
  for (; trycount < 4; ++trycount) {
    switch (arrowAlign) {
      case TOP | LEFT:
        bounds.x = target.x + target.w;
        bounds.y = target.y + target.h;
        break;
      case TOP | RIGHT:
        bounds.x = target.x - bounds.w;
        bounds.y = target.y + target.h;
        break;
      case BOTTOM | LEFT:
        bounds.x = target.x + target.w;
        bounds.y = target.y - bounds.h;
        break;
      case BOTTOM | RIGHT:
        bounds.x = target.x - bounds.w;
        bounds.y = target.y - bounds.h;
        break;
      case TOP:
        bounds.x = target.x + target.w / 2 - bounds.w / 2;
        bounds.y = target.y + target.h;
        break;
      case BOTTOM:
        bounds.x = target.x + target.w / 2 - bounds.w / 2;
        bounds.y = target.y - bounds.h;
        break;
      case LEFT:
        bounds.x = target.x + target.w;
        bounds.y = target.y + target.h / 2 - bounds.h / 2;
        break;
      case RIGHT:
        bounds.x = target.x - bounds.w;
        bounds.y = target.y + target.h / 2 - bounds.h / 2;
        break;
    }

    gfx::Size displaySize = display->size();
    bounds.x = std::clamp(bounds.x, 0, displaySize.w - bounds.w);
    bounds.y = std::clamp(bounds.y, 0, displaySize.h - bounds.h);

    if (target.intersects(bounds)) {
      switch (trycount) {
        case 0:
        case 2:
          // Switch position
          if (arrowAlign & (TOP | BOTTOM))
            arrowAlign ^= TOP | BOTTOM;
          if (arrowAlign & (LEFT | RIGHT))
            arrowAlign ^= LEFT | RIGHT;
          break;
        case 1:
          // Rotate positions
          if (arrowAlign & (TOP | LEFT))
            arrowAlign ^= TOP | LEFT;
          if (arrowAlign & (BOTTOM | RIGHT))
            arrowAlign ^= BOTTOM | RIGHT;
          break;
      }
    }
    else
      break;
  }

  return arrowAlign;
}

void fit_bounds(const Display* parentDisplay,
                Window* window,
                const gfx::Rect& candidateBoundsRelativeToParentDisplay,
                std::function<void(const gfx::Rect& workarea,
                                   gfx::Rect& bounds,
                                   std::function<gfx::Rect(Widget*)> getWidgetBounds)> fitLogic)
{
  gfx::Point pos = candidateBoundsRelativeToParentDisplay.origin();

  if (get_multiple_displays() && window->shouldCreateNativeWindow()) {
    const os::Window* nativeWindow = const_cast<ui::Display*>(parentDisplay)->nativeWindow();
    // Limit to the current screen workarea (instead of using all the
    // available workarea between all monitors, get_workarea_region())
    const gfx::Rect workarea = nativeWindow->screen()->workarea();
    const int scale = nativeWindow->scale();

    // Screen frame bounds
    gfx::Rect frame(nativeWindow->pointToScreen(pos),
                    candidateBoundsRelativeToParentDisplay.size() * scale);

    if (fitLogic)
      fitLogic(workarea, frame, [](Widget* widget) { return widget->boundsOnScreen(); });

    frame.x = std::clamp(frame.x, workarea.x, std::max(workarea.x, workarea.x2() - frame.w));
    frame.y = std::clamp(frame.y, workarea.y, std::max(workarea.y, workarea.y2() - frame.h));

    // Set frame bounds directly
    pos = nativeWindow->pointFromScreen(frame.origin());
    window->setBounds(gfx::Rect(pos.x, pos.y, frame.w / scale, frame.h / scale));
    window->loadNativeFrame(frame);

    if (window->isVisible()) {
      if (window->ownDisplay())
        window->display()->nativeWindow()->setFrame(frame);
    }
  }
  else {
    const gfx::Rect displayBounds(parentDisplay->size());
    gfx::Rect frame(candidateBoundsRelativeToParentDisplay);

    if (fitLogic)
      fitLogic(displayBounds, frame, [](Widget* widget) { return widget->bounds(); });

    frame.x = std::clamp(frame.x, 0, std::max(0, displayBounds.w - frame.w));
    frame.y = std::clamp(frame.y, 0, std::max(0, displayBounds.h - frame.h));

    window->setBounds(frame);
  }
}

// Limit window position using the union of all workareas
//
// TODO at least the title bar should be visible so we can
//      resize it, because workareas can form an irregular shape
//      (not rectangular) the calculation is a little more
//      complex
void limit_with_workarea(Display* parentDisplay, gfx::Rect& frame)
{
  if (!get_multiple_displays())
    return;

  ASSERT(parentDisplay);

  gfx::Rect waBounds = parentDisplay->nativeWindow()->screen()->workarea();
  if (frame.x < waBounds.x)
    frame.x = waBounds.x;
  if (frame.y < waBounds.y)
    frame.y = waBounds.y;
  if (frame.x2() > waBounds.x2()) {
    frame.x -= frame.x2() - waBounds.x2();
    if (frame.x < waBounds.x) {
      frame.x = waBounds.x;
      frame.w = waBounds.w;
    }
  }
  if (frame.y2() > waBounds.y2()) {
    frame.y -= frame.y2() - waBounds.y2();
    if (frame.y < waBounds.y) {
      frame.y = waBounds.y;
      frame.h = waBounds.h;
    }
  }
}

} // namespace ui
