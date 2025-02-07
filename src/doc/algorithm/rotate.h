// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_ALGORITHM_ROTATE_H_INCLUDED
#define DOC_ALGORITHM_ROTATE_H_INCLUDED
#pragma once

namespace doc {
class Image;

namespace algorithm {

void scale_image(Image* dst,
                 const Image* src,
                 int dst_x,
                 int dst_y,
                 int dst_w,
                 int dst_h,
                 int src_x,
                 int src_y,
                 int src_w,
                 int src_h);

void rotate_image(Image* dst,
                  const Image* src,
                  int x,
                  int y,
                  int w,
                  int h,
                  int cx,
                  int cy,
                  double angle);

void parallelogram(Image* dst,
                   const Image* src,
                   const Image* mask,
                   int x1,
                   int y1,
                   int x2,
                   int y2,
                   int x3,
                   int y3,
                   int x4,
                   int y4);

} // namespace algorithm
} // namespace doc

#endif
