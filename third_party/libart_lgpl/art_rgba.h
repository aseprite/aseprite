/*
 * art_rgba.h: Functions for manipulating RGBA pixel data.
 *
 * Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 2000 Raph Levien
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

#ifndef __ART_RGBA_H__
#define __ART_RGBA_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void
art_rgba_rgba_composite (art_u8 *dst, const art_u8 *src, int n);

void
art_rgba_fill_run (art_u8 *buf, art_u8 r, art_u8 g, art_u8 b, int n);

void
art_rgba_run_alpha (art_u8 *buf, art_u8 r, art_u8 g, art_u8 b, int alpha, int n);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
