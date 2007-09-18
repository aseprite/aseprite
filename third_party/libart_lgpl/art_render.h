/*
 * art_render.h: Modular rendering architecture.
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

#ifndef __ART_RENDER_H__
#define __ART_RENDER_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Render object */

#ifndef ART_MAX_DEPTH
#define ART_MAX_DEPTH 16
#endif

#if ART_MAX_DEPTH == 16
typedef art_u16 ArtPixMaxDepth;
#define ART_PIX_MAX_FROM_8(x) ((x) | ((x) << 8))
#define ART_PIX_8_FROM_MAX(x) (((x) + 0x80 - (((x) + 0x80) >> 8)) >> 8)
#else
#if ART_MAX_DEPTH == 8
typedef art_u8 ArtPixMaxDepth;
#define ART_PIX_MAX_FROM_8(x) (x)
#define ART_PIX_8_FROM_MAX(x) (x)
#else
#error ART_MAX_DEPTH must be either 8 or 16
#endif
#endif

#define ART_MAX_CHAN 16

typedef struct _ArtRender ArtRender;
typedef struct _ArtRenderCallback ArtRenderCallback;
typedef struct _ArtRenderMaskRun ArtRenderMaskRun;
typedef struct _ArtImageSource ArtImageSource;
typedef struct _ArtMaskSource ArtMaskSource;

typedef enum {
  ART_ALPHA_NONE      = 0,
  ART_ALPHA_SEPARATE  = 1,
  ART_ALPHA_PREMUL    = 2
} ArtAlphaType;

typedef enum {
  ART_COMPOSITE_NORMAL,
  ART_COMPOSITE_MULTIPLY,
  /* todo: more */
  ART_COMPOSITE_CUSTOM
} ArtCompositingMode;

typedef enum {
  ART_IMAGE_SOURCE_CAN_CLEAR = 1,
  ART_IMAGE_SOURCE_CAN_COMPOSITE = 2
} ArtImageSourceFlags;

struct _ArtRenderMaskRun {
  int x;
  int alpha;
};

struct _ArtRenderCallback {
  void (*render) (ArtRenderCallback *self, ArtRender *render,
		  art_u8 *dest, int y);
  void (*done) (ArtRenderCallback *self, ArtRender *render);
};

struct _ArtImageSource {
  ArtRenderCallback super;
  void (*negotiate) (ArtImageSource *self, ArtRender *render,
		     ArtImageSourceFlags *p_flags,
		     int *p_buf_depth, ArtAlphaType *p_alpha_type);
};

struct _ArtMaskSource {
  ArtRenderCallback super;
  int (*can_drive) (ArtMaskSource *self, ArtRender *render);
  /* For each mask source, ::prepare() is invoked if it is not
     a driver, or ::invoke_driver() if it is. */
  void (*invoke_driver) (ArtMaskSource *self, ArtRender *render);
  void (*prepare) (ArtMaskSource *self, ArtRender *render, art_boolean first);
};

struct _ArtRender {
  /* parameters of destination image */
  int x0, y0;
  int x1, y1;
  art_u8 *pixels;
  int rowstride;
  int n_chan;
  int depth;
  ArtAlphaType alpha_type;

  art_boolean clear;
  ArtPixMaxDepth clear_color[ART_MAX_CHAN + 1];
  art_u32 opacity; /* [0..0x10000] */

  ArtCompositingMode compositing_mode;

  ArtAlphaGamma *alphagamma;

  art_u8 *alpha_buf;

  /* parameters of intermediate buffer */
  int buf_depth;
  ArtAlphaType buf_alpha;
  art_u8 *image_buf;

  /* driving alpha scanline data */
  /* A "run" is a contiguous sequence of x values with the same alpha value. */
  int n_run;
  ArtRenderMaskRun *run;

  /* A "span" is a contiguous sequence of x values with non-zero alpha. */
  int n_span;
  int *span_x;

  art_boolean need_span;
};

ArtRender *
art_render_new (int x0, int y0, int x1, int y1,
		art_u8 *pixels, int rowstride,
		int n_chan, int depth, ArtAlphaType alpha_type,
		ArtAlphaGamma *alphagamma);

void
art_render_invoke (ArtRender *render);

void
art_render_clear (ArtRender *render, const ArtPixMaxDepth *clear_color);

void
art_render_clear_rgb (ArtRender *render, art_u32 clear_rgb);

void
art_render_mask_solid (ArtRender *render, int opacity);

void
art_render_image_solid (ArtRender *render, ArtPixMaxDepth *color);

/* The next two functions are for custom mask sources only. */
void
art_render_add_mask_source (ArtRender *render, ArtMaskSource *mask_source);

void
art_render_invoke_callbacks (ArtRender *render, art_u8 *dest, int y);

/* The following function is for custom image sources only. */
void
art_render_add_image_source (ArtRender *render, ArtImageSource *image_source);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ART_RENDER_H__ */

