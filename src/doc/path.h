// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_PATH_H_INCLUDED
#define DOC_PATH_H_INCLUDED
#pragma once

#error This file is deprecated

#include "doc/object.h"
#include <string>

namespace doc {

  class Image;
  struct _ArtBpath;

  // Path join style
  enum {
    PATH_JOIN_MITER,
    PATH_JOIN_ROUND,
    PATH_JOIN_BEVEL,
  };

  // Path cap type
  enum {
    PATH_CAP_BUTT,
    PATH_CAP_ROUND,
    PATH_CAP_SQUARE,
  };

  class Path : public Object {
  public:
    std::string name;
    int join, cap;
    int size, end;
    struct _ArtBpath *bpath;

    explicit Path(const char* name);
    explicit Path(const Path& path);
    virtual ~Path();
  };

  // void path_union(Path* path, Path* op);
  // void path_intersect(Path* path, Path* op);
  // void path_diff(Path* path, Path* op);
  // void path_minus(Path* path, Path* op);

  void path_set_join(Path* path, int join);
  void path_set_cap(Path* path, int cap);

  void path_moveto(Path* path, double x, double y);
  void path_lineto(Path* path, double x, double y);
  void path_curveto(Path* path,
                    double control_x1, double control_y1,
                    double control_x2, double control_y2,
                    double end_x, double end_y);
  void path_close(Path* path);

  void path_move(Path* path, double x, double y);

  void path_stroke(Path* path, Image* image, int color, double brush_size);
  void path_fill(Path* path, Image* image, int color);

} // namespace doc

#endif
