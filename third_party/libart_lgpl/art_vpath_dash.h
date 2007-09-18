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

#ifndef __ART_VPATH_DASH_H__
#define __ART_VPATH_DASH_H__

/* Apply a dash style to a vector path. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _ArtVpathDash ArtVpathDash;

struct _ArtVpathDash {
  double offset;
  int n_dash;
  double *dash;
};

ArtVpath *
art_vpath_dash (const ArtVpath *vpath, const ArtVpathDash *dash);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_VPATH_DASH_H__ */
