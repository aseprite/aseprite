/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1998 Raph Levien
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <math.h>
#include "art_misc.h"
#include "art_rect.h"

#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif /* MAX */

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif /* MIN */

/* rectangle primitives stolen from gzilla */

/**
 * art_irect_copy: Make a copy of an integer rectangle.
 * @dest: Where the copy is stored.
 * @src: The source rectangle.
 *
 * Copies the rectangle.
 **/
void
art_irect_copy (ArtIRect *dest, const ArtIRect *src) {
  dest->x0 = src->x0;
  dest->y0 = src->y0;
  dest->x1 = src->x1;
  dest->y1 = src->y1;
}

/**
 * art_irect_union: Find union of two integer rectangles.
 * @dest: Where the result is stored.
 * @src1: A source rectangle.
 * @src2: Another source rectangle.
 *
 * Finds the smallest rectangle that includes @src1 and @src2.
 **/
void
art_irect_union (ArtIRect *dest, const ArtIRect *src1, const ArtIRect *src2) {
  if (art_irect_empty (src1)) {
    art_irect_copy (dest, src2);
  } else if (art_irect_empty (src2)) {
    art_irect_copy (dest, src1);
  } else {
    dest->x0 = MIN (src1->x0, src2->x0);
    dest->y0 = MIN (src1->y0, src2->y0);
    dest->x1 = MAX (src1->x1, src2->x1);
    dest->y1 = MAX (src1->y1, src2->y1);
  }
}

/**
 * art_irect_intersection: Find intersection of two integer rectangles.
 * @dest: Where the result is stored.
 * @src1: A source rectangle.
 * @src2: Another source rectangle.
 *
 * Finds the intersection of @src1 and @src2.
 **/
void
art_irect_intersect (ArtIRect *dest, const ArtIRect *src1, const ArtIRect *src2) {
  dest->x0 = MAX (src1->x0, src2->x0);
  dest->y0 = MAX (src1->y0, src2->y0);
  dest->x1 = MIN (src1->x1, src2->x1);
  dest->y1 = MIN (src1->y1, src2->y1);
}

/**
 * art_irect_empty: Determine whether integer rectangle is empty.
 * @src: The source rectangle.
 *
 * Return value: TRUE if @src is an empty rectangle, FALSE otherwise.
 **/
int
art_irect_empty (const ArtIRect *src) {
  return (src->x1 <= src->x0 || src->y1 <= src->y0);
}

#if 0
gboolean irect_point_inside (ArtIRect *rect, GzwPoint *point) {
  return (point->x >= rect->x0 && point->y >= rect->y0 &&
	  point->x < rect->x1 && point->y < rect->y1);
}
#endif

/**
 * art_drect_copy: Make a copy of a rectangle.
 * @dest: Where the copy is stored.
 * @src: The source rectangle.
 *
 * Copies the rectangle.
 **/
void
art_drect_copy (ArtDRect *dest, const ArtDRect *src) {
  dest->x0 = src->x0;
  dest->y0 = src->y0;
  dest->x1 = src->x1;
  dest->y1 = src->y1;
}

/**
 * art_drect_union: Find union of two rectangles.
 * @dest: Where the result is stored.
 * @src1: A source rectangle.
 * @src2: Another source rectangle.
 *
 * Finds the smallest rectangle that includes @src1 and @src2.
 **/
void
art_drect_union (ArtDRect *dest, const ArtDRect *src1, const ArtDRect *src2) {
  if (art_drect_empty (src1)) {
    art_drect_copy (dest, src2);
  } else if (art_drect_empty (src2)) {
    art_drect_copy (dest, src1);
  } else {
    dest->x0 = MIN (src1->x0, src2->x0);
    dest->y0 = MIN (src1->y0, src2->y0);
    dest->x1 = MAX (src1->x1, src2->x1);
    dest->y1 = MAX (src1->y1, src2->y1);
  }
}

/**
 * art_drect_intersection: Find intersection of two rectangles.
 * @dest: Where the result is stored.
 * @src1: A source rectangle.
 * @src2: Another source rectangle.
 *
 * Finds the intersection of @src1 and @src2.
 **/
void
art_drect_intersect (ArtDRect *dest, const ArtDRect *src1, const ArtDRect *src2) {
  dest->x0 = MAX (src1->x0, src2->x0);
  dest->y0 = MAX (src1->y0, src2->y0);
  dest->x1 = MIN (src1->x1, src2->x1);
  dest->y1 = MIN (src1->y1, src2->y1);
}

/**
 * art_irect_empty: Determine whether rectangle is empty.
 * @src: The source rectangle.
 *
 * Return value: TRUE if @src is an empty rectangle, FALSE otherwise.
 **/
int
art_drect_empty (const ArtDRect *src) {
  return (src->x1 <= src->x0 || src->y1 <= src->y0);
}

/**
 * art_drect_affine_transform: Affine transform rectangle.
 * @dst: Where to store the result.
 * @src: The source rectangle.
 * @matrix: The affine transformation.
 *
 * Find the smallest rectangle enclosing the affine transformed @src.
 * The result is exactly the affine transformation of @src when
 * @matrix specifies a rectilinear affine transformation, otherwise it
 * is a conservative approximation.
 **/
void
art_drect_affine_transform (ArtDRect *dst, const ArtDRect *src, const double matrix[6])
{
  double x00, y00, x10, y10;
  double x01, y01, x11, y11;

  x00 = src->x0 * matrix[0] + src->y0 * matrix[2] + matrix[4];
  y00 = src->x0 * matrix[1] + src->y0 * matrix[3] + matrix[5];
  x10 = src->x1 * matrix[0] + src->y0 * matrix[2] + matrix[4];
  y10 = src->x1 * matrix[1] + src->y0 * matrix[3] + matrix[5];
  x01 = src->x0 * matrix[0] + src->y1 * matrix[2] + matrix[4];
  y01 = src->x0 * matrix[1] + src->y1 * matrix[3] + matrix[5];
  x11 = src->x1 * matrix[0] + src->y1 * matrix[2] + matrix[4];
  y11 = src->x1 * matrix[1] + src->y1 * matrix[3] + matrix[5];
  dst->x0 = MIN (MIN (x00, x10), MIN (x01, x11));
  dst->y0 = MIN (MIN (y00, y10), MIN (y01, y11));
  dst->x1 = MAX (MAX (x00, x10), MAX (x01, x11));
  dst->y1 = MAX (MAX (y00, y10), MAX (y01, y11));
}

/**
 * art_drect_to_irect: Convert rectangle to integer rectangle.
 * @dst: Where to store resulting integer rectangle.
 * @src: The source rectangle.
 *
 * Find the smallest integer rectangle that encloses @src.
 **/
void
art_drect_to_irect (ArtIRect *dst, ArtDRect *src)
{
  dst->x0 = floor (src->x0);
  dst->y0 = floor (src->y0);
  dst->x1 = ceil (src->x1);
  dst->y1 = ceil (src->y1);
}
