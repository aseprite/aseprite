// Aseprite
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_SYMMETRIES_H_INCLUDED
#define APP_TOOLS_SYMMETRIES_H_INCLUDED
#pragma once

#include "app/tools/stroke.h"
#include "app/tools/symmetry.h"

#include <memory>

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

class SymmetryCombo : public Symmetry {
public:
  SymmetryCombo(Symmetry* a, Symmetry* b) : m_a(a), m_b(b) { }
  void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                       ToolLoop* loop) override;
private:
  std::unique_ptr<tools::Symmetry> m_a;
  std::unique_ptr<tools::Symmetry> m_b;
};

} // namespace tools
} // namespace app

#endif
