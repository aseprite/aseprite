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

#ifndef __ART_SVP_RENDER_AA_H__
#define __ART_SVP_RENDER_AA_H__

/* The spiffy antialiased renderer for sorted vector paths. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _ArtSVPRenderAAStep ArtSVPRenderAAStep;
typedef struct _ArtSVPRenderAAIter ArtSVPRenderAAIter;

struct _ArtSVPRenderAAStep {
  int x;
  int delta; /* stored with 16 fractional bits */
};

ArtSVPRenderAAIter *
art_svp_render_aa_iter (const ArtSVP *svp,
			int x0, int y0, int x1, int y1);

void
art_svp_render_aa_iter_step (ArtSVPRenderAAIter *iter, int *p_start,
			     ArtSVPRenderAAStep **p_steps, int *p_n_steps);

void
art_svp_render_aa_iter_done (ArtSVPRenderAAIter *iter);

void
art_svp_render_aa (const ArtSVP *svp,
		   int x0, int y0, int x1, int y1,
		   void (*callback) (void *callback_data,
				     int y,
				     int start,
				     ArtSVPRenderAAStep *steps, int n_steps),
		   void *callback_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_SVP_RENDER_AA_H__ */
