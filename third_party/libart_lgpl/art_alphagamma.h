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

#ifndef __ART_ALPHAGAMMA_H__
#define __ART_ALPHAGAMMA_H__

/* Alphagamma tables */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef LIBART_COMPILATION
#include "art_misc.h"
#else
#include <libart_lgpl/art_misc.h>
#endif

typedef struct _ArtAlphaGamma ArtAlphaGamma;

struct _ArtAlphaGamma {
  /*< private >*/
  double gamma;
  int invtable_size;
  int table[256];
  art_u8 invtable[1];
};

ArtAlphaGamma *
art_alphagamma_new (double gamma);

void
art_alphagamma_free (ArtAlphaGamma *alphagamma);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_SVP_H__ */
