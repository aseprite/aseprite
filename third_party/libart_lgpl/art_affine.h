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

#ifndef __ART_AFFINE_H__
#define __ART_AFFINE_H__

#ifdef LIBART_COMPILATION
#include "art_point.h"
#else
#include <libart_lgpl/art_point.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void
art_affine_point (ArtPoint *dst, const ArtPoint *src,
		  const double affine[6]);

void
art_affine_invert (double dst_affine[6], const double src_affine[6]);

/* flip the matrix, FALSE, FALSE is a simple copy operation, and
   TRUE, TRUE equals a rotation by 180 degrees */
void
art_affine_flip (double dst_affine[6], const double src_affine[6],
                 int horz, int vert);

void
art_affine_to_string (char str[128], const double src[6]);

void
art_affine_multiply (double dst[6],
		     const double src1[6], const double src2[6]);

/* set up the identity matrix */
void
art_affine_identity (double dst[6]);

/* set up a scaling matrix */
void
art_affine_scale (double dst[6], double sx, double sy);

/* set up a rotation matrix; theta is given in degrees */
void
art_affine_rotate (double dst[6], double theta);

/* set up a shearing matrix; theta is given in degrees */
void
art_affine_shear (double dst[6], double theta);

/* set up a translation matrix */
void
art_affine_translate (double dst[6], double tx, double ty);


/* find the affine's "expansion factor", i.e. the scale amount */
double
art_affine_expansion (const double src[6]);

/* Determine whether the affine transformation is rectilinear,
   i.e. whether a rectangle aligned to the grid is transformed into
   another rectangle aligned to the grid. */
int
art_affine_rectilinear (const double src[6]);

/* Determine whether two affine transformations are equal within grid allignment */
int
art_affine_equal (double matrix1[6], double matrix2[6]);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_AFFINE_H__ */
