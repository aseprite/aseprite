/* ASEPRITE
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

#ifndef RASTER_BLEND_H_INCLUDED
#define RASTER_BLEND_H_INCLUDED

#define INT_MULT(a, b, t)                               \
  ((t) = (a) * (b) + 0x80, ((((t) >> 8) + (t)) >> 8))

namespace raster {

  enum {
    BLEND_MODE_NORMAL,
    BLEND_MODE_COPY,
    BLEND_MODE_MAX,
  };

  typedef int (*BLEND_COLOR)(int back, int front, int opacity);

  extern BLEND_COLOR _rgba_blenders[];
  extern BLEND_COLOR _graya_blenders[];

  int _rgba_blend_normal(int back, int front, int opacity);
  int _rgba_blend_copy(int back, int front, int opacity);
  int _rgba_blend_forpath(int back, int front, int opacity);
  int _rgba_blend_merge(int back, int front, int opacity);

  int _graya_blend_normal(int back, int front, int opacity);
  int _graya_blend_copy(int back, int front, int opacity);
  int _graya_blend_forpath(int back, int front, int opacity);
  int _graya_blend_merge(int back, int front, int opacity);

} // namespace raster

#endif
