// Aseprite
// Copyright (C) 2015-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/symmetries.h"

#include "app/tools/point_shape.h"
#include "app/tools/stroke.h"
#include "app/tools/tool_loop.h"
#include "doc/brush.h"

namespace app {
namespace tools {

void HorizontalSymmetry::generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                                         ToolLoop* loop)
{
  int adjust;
  if (loop->getPointShape()->isFloodFill()) {
    adjust = 1;
  }
  else {
    // TODO This adjustment is not valid for brush centers that are
    //      not in the default Brush::center(), we'll fix this later
    //      (we should flip the brush center+image+bitmap or just do
    //      the symmetry of all pixels)
    adjust = ((loop->getBrush()->bounds().w % 2) == 0 ? 2: 1);
  }

  strokes.push_back(mainStroke);

  Stroke stroke2;
  for (const auto& pt : mainStroke)
    stroke2.addPoint(gfx::Point(m_x - (pt.x - m_x + adjust), pt.y));
  strokes.push_back(stroke2);
}

void VerticalSymmetry::generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                                       ToolLoop* loop)
{
  int adjust;
  if (loop->getPointShape()->isFloodFill())
    adjust = 1;
  else
    adjust = ((loop->getBrush()->bounds().h % 2) == 0 ? 2: 1);

  strokes.push_back(mainStroke);

  Stroke stroke2;
  for (const auto& pt : mainStroke)
    stroke2.addPoint(gfx::Point(pt.x, m_y - (pt.y - m_y + adjust)));
  strokes.push_back(stroke2);
}

void SymmetryCombo::generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                                    ToolLoop* loop)
{
  Strokes strokes0;
  m_a->generateStrokes(mainStroke, strokes0, loop);
  for (const Stroke& stroke : strokes0)
    m_b->generateStrokes(stroke, strokes, loop);
}

} // namespace tools
} // namespace app
