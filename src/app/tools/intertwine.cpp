// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/intertwine.h"

#include "app/tools/controller.h"
#include "app/tools/point_shape.h"
#include "app/tools/stroke.h"
#include "app/tools/symmetry.h"
#include "app/tools/tool_loop.h"
#include "doc/algo.h"

namespace app {
namespace tools {

using namespace gfx;
using namespace doc;

gfx::Rect Intertwine::getStrokeBounds(ToolLoop* loop, const Stroke& stroke)
{
  return stroke.bounds();
}

void Intertwine::doPointshapePoint(int x, int y, ToolLoop* loop)
{
  Symmetry* symmetry = loop->getSymmetry();
  if (symmetry) {
    // Convert the point to the sprite position so we can apply the
    // symmetry transformation.
    Stroke main_stroke;
    main_stroke.addPoint(Point(x, y));

    Strokes strokes;
    symmetry->generateStrokes(main_stroke, strokes, loop);
    for (const auto& stroke : strokes) {
      // We call transformPoint() moving back each point to the cel
      // origin.
      loop->getPointShape()->transformPoint(
        loop, stroke[0].x, stroke[0].y);
    }
  }
  else {
    loop->getPointShape()->transformPoint(loop, x, y);
  }
}

void Intertwine::doPointshapeHline(int x1, int y, int x2, ToolLoop* loop)
{
  algo_line_perfect(x1, y, x2, y, loop, (AlgoPixel)doPointshapePoint);
}

void Intertwine::doPointshapeLine(int x1, int y1, int x2, int y2, ToolLoop* loop)
{
  if (// When "Snap Angle" in being used or...
      (int(loop->getModifiers()) & int(ToolLoopModifiers::kSquareAspect)) ||
      // "Snap to Grid" is enabled
      (loop->getController()->canSnapToGrid() && loop->getSnapToGrid())) {
    // We prefer the perfect pixel lines that matches grid tiles
    algo_line_perfect(x1, y1, x2, y2, loop, (AlgoPixel)doPointshapePoint);
  }
  else {
    // In other case we use the regular algorithm that is useful to
    // draw continuous lines/strokes.
    algo_line_continuous(x1, y1, x2, y2, loop, (AlgoPixel)doPointshapePoint);
  }
}

} // namespace tools
} // namespace app
