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

#ifndef __ART_SVP_WIND_H__
#define __ART_SVP_WIND_H__

/* Primitive intersection and winding number operations on sorted
   vector paths. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
  ART_WIND_RULE_NONZERO,
  ART_WIND_RULE_INTERSECT,
  ART_WIND_RULE_ODDEVEN,
  ART_WIND_RULE_POSITIVE
} ArtWindRule;

ArtSVP *
art_svp_uncross (ArtSVP *vp);

ArtSVP *
art_svp_rewind_uncrossed (ArtSVP *vp, ArtWindRule rule);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_SVP_WIND_H__ */
