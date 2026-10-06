// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RENDER_CANVAS_VIEW_H_INCLUDED
#define APP_RENDER_CANVAS_VIEW_H_INCLUDED
#pragma once

#include "gfx/rect.h"
#include "render/projection.h"

namespace app {

class CanvasView {
public:
  virtual ~CanvasView() {}

  // Zoom/scale used to render the canvas on this view.
  virtual const render::Projection& cvProjection() const = 0;

  // Position of the canvas in the relative to the Viewport/Editor position.
  virtual gfx::Rect cvCanvasBounds() = 0;
};

} // namespace app

#endif
