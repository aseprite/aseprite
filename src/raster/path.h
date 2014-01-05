/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef RASTER_PATH_H_INCLUDED
#define RASTER_PATH_H_INCLUDED

#error This file is deprecated

#include "raster/object.h"
#include <string>

namespace raster {

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

} // namespace raster

#endif
