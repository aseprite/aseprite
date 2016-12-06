// Aseprite
// Copyright (C) 2015-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_SYMMETRIES_H_INCLUDED
#define APP_TOOLS_SYMMETRIES_H_INCLUDED
#pragma once

#include "app/tools/stroke.h"
#include "app/tools/symmetry.h"

namespace app {
namespace tools {

class HorizontalSymmetry : public Symmetry {
public:
  HorizontalSymmetry(double x) : m_x(x) { }
  void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                       ToolLoop* loop) override;
private:
  double m_x;
};

class VerticalSymmetry : public Symmetry {
public:
  VerticalSymmetry(double y) : m_y(y) { }
  void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                       ToolLoop* loop) override;
private:
  double m_y;
};

} // namespace tools
} // namespace app

#endif
