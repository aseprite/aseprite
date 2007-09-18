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

#ifndef __ART_UTA_H__
#define __ART_UTA_H__

/* Basic data structures and constructors for microtile arrays */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef art_u32 ArtUtaBbox;
typedef struct _ArtUta ArtUta;

#define ART_UTA_BBOX_CONS(x0, y0, x1, y1) (((x0) << 24) | ((y0) << 16) | \
				       ((x1) << 8) | (y1))

#define ART_UTA_BBOX_X0(ub) ((ub) >> 24)
#define ART_UTA_BBOX_Y0(ub) (((ub) >> 16) & 0xff)
#define ART_UTA_BBOX_X1(ub) (((ub) >> 8) & 0xff)
#define ART_UTA_BBOX_Y1(ub) ((ub) & 0xff)

#define ART_UTILE_SHIFT 5
#define ART_UTILE_SIZE (1 << ART_UTILE_SHIFT)

/* Coordinates are shifted right by ART_UTILE_SHIFT wrt the real
   coordinates. */
struct _ArtUta {
  int x0;
  int y0;
  int width;
  int height;
  ArtUtaBbox *utiles;
};

ArtUta *
art_uta_new (int x0, int y0, int x1, int y1);

ArtUta *
art_uta_new_coords (int x0, int y0, int x1, int y1);

void
art_uta_free (ArtUta *uta);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_UTA_H__ */
