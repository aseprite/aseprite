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

#ifndef __ART_FILTERLEVEL_H__
#define __ART_FILTERLEVEL_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  ART_FILTER_NEAREST,
  ART_FILTER_TILES,
  ART_FILTER_BILINEAR,
  ART_FILTER_HYPER
} ArtFilterLevel;

/* NEAREST is nearest neighbor. It is the fastest and lowest quality.

   TILES is an accurate simulation of the PostScript image operator
   without any interpolation enabled; each pixel is rendered as a tiny
   parallelogram of solid color, the edges of which are implemented
   with antialiasing. It resembles nearest neighbor for enlargement,
   and bilinear for reduction.

   BILINEAR is bilinear interpolation. For enlargement, it is
   equivalent to point-sampling the ideal bilinear-interpolated
   image. For reduction, it is equivalent to laying down small tiles
   and integrating over the coverage area.

   HYPER is the highest quality reconstruction function. It is derived
   from the hyperbolic filters in Wolberg's "Digital Image Warping,"
   and is formally defined as the hyperbolic-filter sampling the ideal
   hyperbolic-filter interpolated image (the filter is designed to be
   idempotent for 1:1 pixel mapping). It is the slowest and highest
   quality.

   Note: at this stage of implementation, most filter modes are likely
   not to be implemented.

   Note: cubic filtering is missing from this list, because there isn't
   much point - hyper is just as fast to implement and slightly better
   in quality.

*/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_PATHCODE_H__ */
