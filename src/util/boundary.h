/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Adapted to ASE by David Capello (2003-2010)
 * See "LICENSE.txt" for more information.
 */

#ifndef UTIL_BOUNDARY_H_INCLUDED
#define UTIL_BOUNDARY_H_INCLUDED

#ifdef __cplusplus
  extern "C" {
#endif

class Image;

typedef enum
{
  WithinBounds,
  IgnoreBounds
} BoundaryType;

typedef struct _BoundSeg
{
  int x1, y1, x2, y2;
  unsigned open : 1;
  unsigned visited : 1;
} BoundSeg;

BoundSeg* find_mask_boundary(const Image* maskPR, int* num_elems,
			     BoundaryType type, int x1, int y1, int x2, int y2);
BoundSeg* sort_boundary(BoundSeg* segs, int num_segs, int* num_groups);

void boundary_exit();

#ifdef __cplusplus
  }
#endif

#endif
