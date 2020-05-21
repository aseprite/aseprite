// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
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
#include "base/pi.h"
#include "doc/algo.h"

#include <cmath>

namespace app {
namespace tools {

using namespace gfx;
using namespace doc;

Intertwine::LineData::LineData(ToolLoop* loop,
                               const Stroke::Pt& a,
                               const Stroke::Pt& b)
  : loop(loop)
  , a(a)
  , b(b)
  , pt(a)
{
  const int steps = std::max(std::abs(b.x - a.x),
                             std::abs(b.y - a.y))+1;
  t = 0.0f;
  step = 1.0f / steps;
}

void Intertwine::LineData::doStep(int x, int y)
{
  t += step;
  const float ti = 1.0f-t;

  pt.x = x;
  pt.y = y;
  pt.size = ti*a.size + t*b.size;
  pt.angle = ti*a.angle + t*b.angle;
  pt.gradient = ti*a.gradient + t*b.gradient;
}

gfx::Rect Intertwine::getStrokeBounds(ToolLoop* loop, const Stroke& stroke)
{
  return stroke.bounds();
}

// static
void Intertwine::doPointshapeStrokePt(const Stroke::Pt& pt, ToolLoop* loop)
{
  Symmetry* symmetry = loop->getSymmetry();
  if (symmetry) {
    // Convert the point to the sprite position so we can apply the
    // symmetry transformation.
    Stroke main_stroke;
    main_stroke.addPoint(pt);

    Strokes strokes;
    symmetry->generateStrokes(main_stroke, strokes, loop);
    for (const auto& stroke : strokes) {
      // We call transformPoint() moving back each point to the cel
      // origin.
      loop->getPointShape()->transformPoint(loop, stroke[0]);
    }
  }
  else {
    loop->getPointShape()->transformPoint(loop, pt);
  }
}

// static
void Intertwine::doPointshapePoint(int x, int y, ToolLoop* loop)
{
  Stroke::Pt pt(x, y);
  pt.size = loop->getBrush()->size();
  pt.angle = loop->getBrush()->angle();
  doPointshapeStrokePt(pt, loop);
}

// static
void Intertwine::doPointshapePointDynamics(int x, int y, Intertwine::LineData* data)
{
  data->doStep(x, y);
  doPointshapeStrokePt(data->pt, data->loop);
}

// static
void Intertwine::doPointshapeHline(int x1, int y, int x2, ToolLoop* loop)
{
  algo_line_perfect(x1, y, x2, y, loop, (AlgoPixel)doPointshapePoint);
}

// static
void Intertwine::doPointshapeLineWithoutDynamics(int x1, int y1, int x2, int y2, ToolLoop* loop)
{
  Stroke::Pt a(x1, y1);
  Stroke::Pt b(x2, y2);
  a.size = b.size = loop->getBrush()->size();
  a.angle = b.angle = loop->getBrush()->angle();
  doPointshapeLine(a, b, loop);
}

void Intertwine::doPointshapeLine(const Stroke::Pt& a,
                                  const Stroke::Pt& b, ToolLoop* loop)
{
  doc::AlgoLineWithAlgoPixel algo = getLineAlgo(loop, a, b);
  LineData lineData(loop, a, b);
  algo(a.x, a.y, b.x, b.y, (void*)&lineData, (AlgoPixel)doPointshapePointDynamics);
}

// static
doc::AlgoLineWithAlgoPixel Intertwine::getLineAlgo(ToolLoop* loop,
                                                   const Stroke::Pt& a,
                                                   const Stroke::Pt& b)
{
  bool needsFixForLineBrush = false;
  if (loop->getBrush()->type() == kLineBrushType) {
    if ((a.angle != 0.0f || b.angle != 0.0f) &&
        (a.angle != b.angle)) {
      needsFixForLineBrush = true;
    }
    else {
      int angle = a.angle;
      int p = SGN(b.x - a.x);
      int q = SGN(a.y - b.y);
      float rF = std::cos(PI * angle / 180);
      float sF = std::sin(PI * angle / 180);
      int r = SGN(rF);
      int s = SGN(sF);
      needsFixForLineBrush = ((p == q && r != s) ||
                              (p != q && r == s));
    }
  }

  if (// When "Snap Angle" in being used or...
      (int(loop->getModifiers()) & int(ToolLoopModifiers::kSquareAspect)) ||
      // "Snap to Grid" is enabled
      (loop->getController()->canSnapToGrid() && loop->getSnapToGrid())) {
    // We prefer the perfect pixel lines that matches grid tiles
    return (needsFixForLineBrush ? algo_line_perfect_with_fix_for_line_brush:
                                   algo_line_perfect);
  }
  else {
    // In other case we use the regular algorithm that is useful to
    // draw continuous lines/strokes.
    return (needsFixForLineBrush ? algo_line_continuous_with_fix_for_line_brush:
                                   algo_line_continuous);
  }
}

} // namespace tools
} // namespace app
