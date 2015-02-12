// The GIMP -- an image manipulation program
// Copyright (C) 1995 Spencer Kimball and Peter Mattis
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// --
//
// Adapted to Aseprite by David Capello (2001-2012)

#ifndef APP_UTIL_BOUNDARY_H_INCLUDED
#define APP_UTIL_BOUNDARY_H_INCLUDED
#pragma once

namespace doc {
  class Image;
}

namespace app {
  using namespace doc;

  enum BoundaryType {
    WithinBounds,
    IgnoreBounds
  };

  struct BoundSeg {
    int x1, y1, x2, y2;
    unsigned open : 1;
    unsigned visited : 1;
  };

  // The returned pointer must be freed using base_free() function.
  BoundSeg* find_mask_boundary(const Image* maskPR, int* num_elems,
                               BoundaryType type, int x1, int y1, int x2, int y2);

  BoundSeg* sort_boundary(BoundSeg* segs, int num_segs, int* num_groups);

  // Call this function when you don't use find_mask_boundary() anymore (e.g. atexit()).
  void boundary_exit();

} // namespace app

#endif
