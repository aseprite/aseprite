// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
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
  double x() { return m_x; }
private:
  double m_x;
};

class VerticalSymmetry : public Symmetry {
public:
  VerticalSymmetry(double y) : m_y(y) { }
  void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                       ToolLoop* loop) override;
  double y() { return m_y; }
private:
  double m_y;
};

class SymmetryCombo : public Symmetry {
public:
  SymmetryCombo(HorizontalSymmetry* a, VerticalSymmetry* b) : m_a(a), m_b(b),m_x(a->x()), m_y(b->y()) { }
  void generateStrokes(const Stroke& mainStroke, Strokes& strokes,
                       ToolLoop* loop) override;
private:
  std::unique_ptr<tools::Symmetry> m_a;
  std::unique_ptr<tools::Symmetry> m_b;
  double m_x;
  double m_y;
};

} // namespace tools
} // namespace app

#endif
