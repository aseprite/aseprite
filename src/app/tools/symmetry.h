// Aseprite
// Copyright (C) 2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_SYMMETRY_H_INCLUDED
#define APP_TOOLS_SYMMETRY_H_INCLUDED
#pragma once

#include "app/tools/stroke.h"

#include <vector>

namespace app {
  namespace tools {

    class ToolLoop;

    // This class controls user input.
    class Symmetry {
    public:
      virtual ~Symmetry() { }

      // The "stroke" must be relative to the sprite origin.
      virtual void generateStrokes(const Stroke& stroke, Strokes& strokes, ToolLoop* loop) = 0;
    };

  } // namespace tools
} // namespace app

#endif
