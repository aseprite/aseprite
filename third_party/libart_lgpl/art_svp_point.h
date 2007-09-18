/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1999 Raph Levien
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

#ifndef __ART_SVP_POINT_H__
#define __ART_SVP_POINT_H__

/* Determine whether a point is inside, or near, an svp. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int
art_svp_point_wind (ArtSVP *svp, double x, double y);

double
art_svp_point_dist (ArtSVP *svp, double x, double y);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_SVP_H__ */




