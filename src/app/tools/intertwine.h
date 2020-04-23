// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_INTERTWINE_H_INCLUDED
#define APP_TOOLS_INTERTWINE_H_INCLUDED
#pragma once

#include "app/tools/stroke.h"
#include "doc/algo.h"
#include "gfx/point.h"
#include "gfx/rect.h"

namespace app {
  namespace tools {
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

      struct LineData {
        ToolLoop* loop;
        Stroke::Pt a, b, pt;
        float t, step;
        LineData(ToolLoop* loop, const Stroke::Pt& a, const Stroke::Pt& b);
        void doStep(int x, int y);
      };

    protected:
      static void doPointshapeStrokePt(const Stroke::Pt& pt, ToolLoop* loop);
      // The given point must be relative to the cel origin.
      static void doPointshapePoint(int x, int y, ToolLoop* loop);
      static void doPointshapePointDynamics(int x, int y, LineData* data);
      static void doPointshapeHline(int x1, int y, int x2, ToolLoop* loop);
      // TODO We should remove this function and always use dynamics
      static void doPointshapeLineWithoutDynamics(int x1, int y1, int x2, int y2, ToolLoop* loop);
      static void doPointshapeLine(const Stroke::Pt& a,
                                   const Stroke::Pt& b, ToolLoop* loop);

      static doc::AlgoLineWithAlgoPixel getLineAlgo(ToolLoop* loop,
                                                    const Stroke::Pt& a,
                                                    const Stroke::Pt& b);
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_INTERTWINE_H_INCLUDED
