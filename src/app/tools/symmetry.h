// Aseprite
// Copyright (C) 2021-2025  Igara Studio S.A.
// Copyright (C) 2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_SYMMETRY_H_INCLUDED
#define APP_TOOLS_SYMMETRY_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "app/tools/stroke.h"
#include "doc/brush.h"

namespace app { namespace tools {

class ToolLoop;

static inline bool does_symmetry_rotate_image(doc::SymmetryIndex symmetry)
{
  return symmetry == doc::SymmetryIndex::ROTATED_90 ||
         symmetry == doc::SymmetryIndex::ROTATED_270 ||
         symmetry == doc::SymmetryIndex::ROT_FLIP_90 ||
         symmetry == doc::SymmetryIndex::ROT_FLIP_270;
}

class Symmetry {
public:
  Symmetry(gen::SymmetryMode symmetryMode, double x, double y)
    : m_symmetryMode(symmetryMode)
    , m_x(x)
    , m_y(y)
  {
  }

  void generateStrokes(const Stroke& stroke, Strokes& strokes, ToolLoop* loop);

  gen::SymmetryMode mode() const { return m_symmetryMode; }

  static gen::SymmetryMode resolveMode(gen::SymmetryMode mode);

private:
  void calculateSymmetricalStroke(const Stroke& refStroke,
                                  Stroke& stroke,
                                  ToolLoop* loop,
                                  const doc::SymmetryIndex symmetry,
                                  const bool isDoubleDiagonalSymmetry = false);

  gen::SymmetryMode m_symmetryMode;
  double m_x, m_y;
};

}} // namespace app::tools

#endif
