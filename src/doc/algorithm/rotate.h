// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ALGORITHM_ROTATE_H_INCLUDED
#define DOC_ALGORITHM_ROTATE_H_INCLUDED
#pragma once

namespace doc {
  class Image;

  namespace algorithm {

    void scale_image(Image* dst, Image* src,
      int x, int y, int w, int h);

    void rotate_image(Image* dst, Image* src,
      int x, int y, int w, int h,
      int cx, int cy, double angle);

    void parallelogram(Image* bmp, Image* sprite,
      int x1, int y1, int x2, int y2,
      int x3, int y3, int x4, int y4);

  } // namespace algorithm
} // namespace doc

#endif
