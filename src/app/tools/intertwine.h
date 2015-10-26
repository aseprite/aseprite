// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_INTERTWINE_H_INCLUDED
#define APP_TOOLS_INTERTWINE_H_INCLUDED
#pragma once

#include "gfx/point.h"

#include <vector>

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
      typedef std::vector<gfx::Point> Points;

      virtual ~Intertwine() { }
      virtual bool snapByAngle() { return false; }
      virtual void prepareIntertwine() { }
      virtual void joinStroke(ToolLoop* loop, const Stroke& stroke) = 0;
      virtual void fillStroke(ToolLoop* loop, const Stroke& stroke) = 0;

    protected:
      static void doPointshapePoint(int x, int y, ToolLoop* loop);
      static void doPointshapeHline(int x1, int y, int x2, ToolLoop* loop);
      static void doPointshapeLine(int x1, int y1, int x2, int y2, ToolLoop* loop);
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_INTERTWINE_H_INCLUDED
