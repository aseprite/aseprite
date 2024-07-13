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
#include <climits>

namespace ui {

// if the number of monitors changes during runtime this could break
static const std::vector<gfx::Rect> get_all_workareas()
{
  std::vector<gfx::Rect> workareas;
  os::ScreenList screens;
  os::instance()->listScreens(screens);
  for (const auto& screen : screens)
    workareas.push_back(screen->workarea());
  return workareas;
}

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
        bounds.x = target.x + target.w/2 - bounds.w/2;
        bounds.y = target.y + target.h;
        break;
      case BOTTOM:
        bounds.x = target.x + target.w/2 - bounds.w/2;
        bounds.y = target.y - bounds.h;
        break;
      case LEFT:
        bounds.x = target.x + target.w;
        bounds.y = target.y + target.h/2 - bounds.h/2;
        break;
      case RIGHT:
        bounds.x = target.x - bounds.w;
        bounds.y = target.y + target.h/2 - bounds.h/2;
        break;
    }

    gfx::Size displaySize = display->size();
    bounds.x = std::clamp(bounds.x, 0, displaySize.w-bounds.w);
    bounds.y = std::clamp(bounds.y, 0, displaySize.h-bounds.h);

    if (target.intersects(bounds)) {
      switch (trycount) {
        case 0:
        case 2:
          // Switch position
          if (arrowAlign & (TOP | BOTTOM)) arrowAlign ^= TOP | BOTTOM;
          if (arrowAlign & (LEFT | RIGHT)) arrowAlign ^= LEFT | RIGHT;
          break;
        case 1:
          // Rotate positions
          if (arrowAlign & (TOP | LEFT)) arrowAlign ^= TOP | LEFT;
          if (arrowAlign & (BOTTOM | RIGHT)) arrowAlign ^= BOTTOM | RIGHT;
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
    // available workarea between all monitors)
    const gfx::Rect workarea = nativeWindow->screen()->workarea();
    const int scale = nativeWindow->scale();

    // Screen frame bounds
    gfx::Rect frame(
      nativeWindow->pointToScreen(pos),
      candidateBoundsRelativeToParentDisplay.size() * scale);

    if (fitLogic)
      fitLogic(workarea, frame, [](Widget* widget){ return widget->boundsOnScreen(); });

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
      fitLogic(displayBounds, frame, [](Widget* widget){ return widget->bounds(); });

    frame.x = std::clamp(frame.x, 0, std::max(0, displayBounds.w - frame.w));
    frame.y = std::clamp(frame.y, 0, std::max(0, displayBounds.h - frame.h));

    window->setBounds(frame);
  }
}

void limit_with_workarea(const gfx::Rect& workareaBounds, gfx::Rect& frame)
{
  if (frame.x < workareaBounds.x) frame.x = workareaBounds.x;
  if (frame.y < workareaBounds.y) frame.y = workareaBounds.y;
  if (frame.x2() > workareaBounds.x2()) {
    frame.x -= frame.x2() - workareaBounds.x2();
    if (frame.x < workareaBounds.x) {
      frame.x = workareaBounds.x;
      frame.w = workareaBounds.w;
    }
  }
  if (frame.y2() > workareaBounds.y2()) {
    frame.y -= frame.y2() - workareaBounds.y2();
    if (frame.y < workareaBounds.y) {
      frame.y = workareaBounds.y;
      frame.h = workareaBounds.h;
    }
  }
}

// since sqrt(a) < sqrt(b) for all a < b, we only have to square to compare
inline int distance_squared(const gfx::Rect& r1, const gfx::Rect& r2) 
{
  int dx = r1.x - r2.x;
  int dy = r1.y - r2.y;
  return dx*dx + dy*dy;
}

void limit_least(gfx::Rect& frame) 
{
  int min_distance = INT_MAX;
  gfx::Rect candidate(frame); // it shouldn't be possible for there to be no workareas but set candidate to a valid Rect anyways

  for (const auto& workarea : get_all_workareas()) {
    // simulate clamping
    gfx::Rect clone(frame);
    limit_with_workarea(workarea, clone);

    // find the least clamped clone
    int distance = distance_squared(clone, frame);
    if (distance < min_distance) {
      min_distance = distance;

      candidate.x = clone.x;
      candidate.y = clone.y;
      candidate.w = clone.w;
      candidate.h = clone.h;
    }
  }

  frame.x = candidate.x;
  frame.y = candidate.y;
  frame.w = candidate.w;
  frame.h = candidate.h;
}

} // namespace ui
