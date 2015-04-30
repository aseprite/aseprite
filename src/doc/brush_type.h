// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_BRUSH_TYPE_H_INCLUDED
#define DOC_BRUSH_TYPE_H_INCLUDED
#pragma once

namespace doc {

  enum BrushType {
    kCircleBrushType = 0,
    kSquareBrushType = 1,
    kLineBrushType = 2,
    kImageBrushType = 3,

    kFirstBrushType = kCircleBrushType,
    kLastBrushType = kImageBrushType,
  };

} // namespace doc

#endif
