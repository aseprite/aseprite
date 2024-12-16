// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_FREEHAND_ALGORITHM_H_INCLUDED
#define APP_TOOLS_FREEHAND_ALGORITHM_H_INCLUDED
#pragma once

namespace app { namespace tools {

enum class FreehandAlgorithm {
  DEFAULT = 0,
  REGULAR = 0,
  PIXEL_PERFECT = 1,
  DOTS = 2,
};

}} // namespace app::tools

#endif
