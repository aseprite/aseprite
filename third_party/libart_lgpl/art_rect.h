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

#ifndef __ART_RECT_H__
#define __ART_RECT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ArtDRect ArtDRect;
typedef struct _ArtIRect ArtIRect;

struct _ArtDRect {
  /*< public >*/
  double x0, y0, x1, y1;
};

struct _ArtIRect {
  /*< public >*/
  int x0, y0, x1, y1;
};

/* Make a copy of the rectangle. */
void art_irect_copy (ArtIRect *dest, const ArtIRect *src);

/* Find the smallest rectangle that includes both source rectangles. */
void art_irect_union (ArtIRect *dest,
		      const ArtIRect *src1, const ArtIRect *src2);

/* Return the intersection of the two rectangles */
void art_irect_intersect (ArtIRect *dest,
			  const ArtIRect *src1, const ArtIRect *src2);

/* Return true if the rectangle is empty. */
int art_irect_empty (const ArtIRect *src);

/* Make a copy of the rectangle. */
void art_drect_copy (ArtDRect *dest, const ArtDRect *src);

/* Find the smallest rectangle that includes both source rectangles. */
void art_drect_union (ArtDRect *dest,
		      const ArtDRect *src1, const ArtDRect *src2);

/* Return the intersection of the two rectangles */
void art_drect_intersect (ArtDRect *dest,
			  const ArtDRect *src1, const ArtDRect *src2);

/* Return true if the rectangle is empty. */
int art_drect_empty (const ArtDRect *src);

void
art_drect_affine_transform (ArtDRect *dst, const ArtDRect *src,
			   const double matrix[6]);

void art_drect_to_irect (ArtIRect *dst, ArtDRect *src);

#ifdef __cplusplus
}
#endif

#endif
