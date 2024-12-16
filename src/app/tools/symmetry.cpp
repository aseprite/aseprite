// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/tools/symmetry.h"

#include "app/tools/point_shape.h"
#include "app/tools/tool_loop.h"

namespace app { namespace tools {

void Symmetry::generateStrokes(const Stroke& stroke, Strokes& strokes, ToolLoop* loop)
{
  Stroke stroke2;
  strokes.push_back(stroke);
  gen::SymmetryMode symmetryMode = loop->getSymmetry()->mode();
  switch (symmetryMode) {
    case gen::SymmetryMode::NONE: ASSERT(false); break;

    case gen::SymmetryMode::HORIZONTAL:
    case gen::SymmetryMode::VERTICAL:
      calculateSymmetricalStroke(stroke, stroke2, loop, symmetryMode);
      strokes.push_back(stroke2);
      break;

    case gen::SymmetryMode::BOTH: {
      calculateSymmetricalStroke(stroke, stroke2, loop, gen::SymmetryMode::HORIZONTAL);
      strokes.push_back(stroke2);

      Stroke stroke3;
      calculateSymmetricalStroke(stroke, stroke3, loop, gen::SymmetryMode::VERTICAL);
      strokes.push_back(stroke3);

      Stroke stroke4;
      calculateSymmetricalStroke(stroke3, stroke4, loop, gen::SymmetryMode::BOTH);
      strokes.push_back(stroke4);
      break;
    }
  }
}

void Symmetry::calculateSymmetricalStroke(const Stroke& refStroke,
                                          Stroke& stroke,
                                          ToolLoop* loop,
                                          gen::SymmetryMode symmetryMode)
{
  int brushSize, brushCenter;
  if (loop->getPointShape()->isFloodFill()) {
    brushSize = 1;
    brushCenter = 0;
  }
  else {
    // TODO we should flip the brush center+image+bitmap or just do
    //      the symmetry of all pixels
    auto brush = loop->getBrush();
    if (symmetryMode == gen::SymmetryMode::HORIZONTAL || symmetryMode == gen::SymmetryMode::BOTH) {
      brushSize = brush->bounds().w;
      brushCenter = brush->center().x;
    }
    else {
      brushSize = brush->bounds().h;
      brushCenter = brush->center().y;
    }
  }

  const bool isDynamic = loop->getDynamics().isDynamic();
  for (const auto& pt : refStroke) {
    if (isDynamic) {
      brushSize = pt.size;
      brushCenter = (brushSize - brushSize % 2) / 2;
    }
    Stroke::Pt pt2 = pt;
    pt2.symmetry = symmetryMode;
    if (symmetryMode == gen::SymmetryMode::HORIZONTAL || symmetryMode == gen::SymmetryMode::BOTH)
      pt2.x = 2 * (m_x + brushCenter) - pt2.x - brushSize;
    else
      pt2.y = 2 * (m_y + brushCenter) - pt2.y - brushSize;
    stroke.addPoint(pt2);
  }
}

}} // namespace app::tools
