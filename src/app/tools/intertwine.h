// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_INTERTWINE_H_INCLUDED
#define APP_TOOLS_INTERTWINE_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "gfx/rect.h"

namespace app {
  namespace tools {
    class Stroke;
    class ToolLoop;

    // Converts a sequence of points in several call to
    // Intertwine::doPointshapePoint(). Basically each implementation
    // says which pixels should be drawn between a sequence of
    // user-defined points.
    class Intertwine {
    public:
      virtual ~Intertwine() { }
      virtual bool snapByAngle() { return false; }
      virtual void prepareIntertwine() { }

      // The given stroke must be relative to the cel origin.
      virtual void joinStroke(ToolLoop* loop, const Stroke& stroke) = 0;
      virtual void fillStroke(ToolLoop* loop, const Stroke& stroke) = 0;

      virtual gfx::Rect getStrokeBounds(ToolLoop* loop, const Stroke& stroke);

    protected:
      // The given point must be relative to the cel origin.
      static void doPointshapePoint(int x, int y, ToolLoop* loop);
      static void doPointshapeHline(int x1, int y, int x2, ToolLoop* loop);
      static void doPointshapeLine(int x1, int y1, int x2, int y2, ToolLoop* loop);
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_INTERTWINE_H_INCLUDED
