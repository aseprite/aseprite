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

#ifndef __ART_UTA_VPATH_H__
#define __ART_UTA_VPATH_H__

/* Basic data structures and constructors for microtile arrays */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ArtUta *
art_uta_from_vpath (const ArtVpath *vec);

/* This is a private function: */
void
art_uta_add_line (ArtUta *uta, double x0, double y0, double x1, double y1,
		  int *rbuf, int rbuf_rowstride);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_UTA_VPATH_H__ */

