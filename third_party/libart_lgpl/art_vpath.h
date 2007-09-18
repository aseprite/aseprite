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

#ifndef __ART_VPATH_H__
#define __ART_VPATH_H__

#ifdef LIBART_COMPILATION
#include "art_rect.h"
#include "art_pathcode.h"
#else
#include <libart_lgpl/art_rect.h>
#include <libart_lgpl/art_pathcode.h>
#endif

/* Basic data structures and constructors for simple vector paths */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _ArtVpath ArtVpath;

/* CURVETO is not allowed! */
struct _ArtVpath {
  ArtPathcode code;
  double x;
  double y;
};

/* Some of the functions need to go into their own modules */

void
art_vpath_add_point (ArtVpath **p_vpath, int *pn_points, int *pn_points_max,
		     ArtPathcode code, double x, double y);

ArtVpath *
art_vpath_new_circle (double x, double y, double r);

ArtVpath *
art_vpath_affine_transform (const ArtVpath *src, const double matrix[6]);

void
art_vpath_bbox_drect (const ArtVpath *vec, ArtDRect *drect);

void
art_vpath_bbox_irect (const ArtVpath *vec, ArtIRect *irect);

ArtVpath *
art_vpath_perturb (ArtVpath *src);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_VPATH_H__ */
