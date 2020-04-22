// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_TOOL_POINT_SHAPE_H_INCLUDED
#define APP_TOOLS_TOOL_POINT_SHAPE_H_INCLUDED
#pragma once

#include "app/tools/stroke.h"
#include "gfx/rect.h"

namespace app {
  namespace tools {
    class ToolLoop;

    // Converts a point to a shape to be drawn
    class PointShape {
    public:
      virtual ~PointShape() { }
      virtual bool isPixel() { return false; }
      virtual bool isFloodFill() { return false; }
      virtual bool isSpray() { return false; }
      virtual void preparePointShape(ToolLoop* loop) { }

      // The x, y position must be relative to the cel/src/dst image origin.
      virtual void transformPoint(ToolLoop* loop, const Stroke::Pt& pt) = 0;
      virtual void getModifiedArea(ToolLoop* loop, int x, int y, gfx::Rect& area) = 0;

    protected:
      // Calls loop->getInk()->inkHline() function for each horizontal-scanline
      // that should be drawn (applying the "tiled" mode loop->getTiledMode())
      static void doInkHline(int x1, int y, int x2, ToolLoop* loop);
    };

  } // namespace tools
} // namespace app

#endif  // APP_TOOLS_TOOL_POINT_SHAPE_H_INCLUDED
