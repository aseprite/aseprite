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

#ifndef __ART_SVP_H__
#define __ART_SVP_H__

/* Basic data structures and constructors for sorted vector paths */

#ifdef LIBART_COMPILATION
#include "art_rect.h"
#include "art_point.h"
#else
#include <libart_lgpl/art_rect.h>
#include <libart_lgpl/art_point.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _ArtSVP ArtSVP;
typedef struct _ArtSVPSeg ArtSVPSeg;

struct _ArtSVPSeg {
  int n_points;
  int dir; /* == 0 for "up", 1 for "down" */
  ArtDRect bbox;
  ArtPoint *points;
};

struct _ArtSVP {
  int n_segs;
  ArtSVPSeg segs[1];
};

int
art_svp_add_segment (ArtSVP **p_vp, int *pn_segs_max,
		     int **pn_points_max,
		     int n_points, int dir, ArtPoint *points,
		     ArtDRect *bbox);

void
art_svp_free (ArtSVP *svp);

int
art_svp_seg_compare (const void *s1, const void *s2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_SVP_H__ */
