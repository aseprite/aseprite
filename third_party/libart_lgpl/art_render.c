/*
 * art_render.c: Modular rendering architecture.
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

#include "art_misc.h"
#include "art_alphagamma.h"
#include "art_rgb.h"

#include "art_render.h"

typedef struct _ArtRenderPriv ArtRenderPriv;

struct _ArtRenderPriv {
  ArtRender super;

  ArtImageSource *image_source;

  int n_mask_source;
  ArtMaskSource **mask_source;

  int n_callbacks;
  ArtRenderCallback **callbacks;
};

ArtRender *
art_render_new (int x0, int y0, int x1, int y1,
		art_u8 *pixels, int rowstride,
		int n_chan, int depth, ArtAlphaType alpha_type,
		ArtAlphaGamma *alphagamma)
{
  ArtRenderPriv *priv;
  ArtRender *result;

  priv = art_new (ArtRenderPriv, 1);
  result = &priv->super;

  if (n_chan > ART_MAX_CHAN)
    {
      art_warn ("art_render_new: n_chan = %d, exceeds %d max\n",
		n_chan, ART_MAX_CHAN);
      return NULL;
    }
  if (depth > ART_MAX_DEPTH)
    {
      art_warn ("art_render_new: depth = %d, exceeds %d max\n",
		depth, ART_MAX_DEPTH);
      return NULL;
    }
  if (x0 >= x1)
    {
      art_warn ("art_render_new: x0 >= x1 (x0 = %d, x1 = %d)\n", x0, x1);
      return NULL;
    }
  result->x0 = x0;
  result->y0 = y0;
  result->x1 = x1;
  result->y1 = y1;
  result->pixels = pixels;
  result->rowstride = rowstride;
  result->n_chan = n_chan;
  result->depth = depth;
  result->alpha_type = alpha_type;

  result->clear = ART_FALSE;
  result->opacity = 0x10000;
  result->compositing_mode = ART_COMPOSITE_NORMAL;
  result->alphagamma = alphagamma;

  result->alpha_buf = NULL;
  result->image_buf = NULL;

  result->run = NULL;
  result->span_x = NULL;

  result->need_span = ART_FALSE;

  priv->image_source = NULL;

  priv->n_mask_source = 0;
  priv->mask_source = NULL;

  return result;
}

/* todo on clear routines: I haven't really figured out what to do
   with clearing the alpha channel. It _should_ be possible to clear
   to an arbitrary RGBA color. */

/**
 * art_render_clear: Set clear color.
 * @clear_color: Color with which to clear dest.
 *
 * Sets clear color, equivalent to actually clearing the destination
 * buffer before rendering. This is the most general form.
 **/
void
art_render_clear (ArtRender *render, const ArtPixMaxDepth *clear_color)
{
  int i;
  int n_ch = render->n_chan + (render->alpha_type != ART_ALPHA_NONE);

  render->clear = ART_TRUE;
  for (i = 0; i < n_ch; i++)
    render->clear_color[i] = clear_color[i];
}

/**
 * art_render_clear_rgb: Set clear color, given in RGB format.
 * @clear_rgb: Clear color, in 0xRRGGBB format.
 *
 * Sets clear color, equivalent to actually clearing the destination
 * buffer before rendering.
 **/
void
art_render_clear_rgb (ArtRender *render, art_u32 clear_rgb)
{
  if (render->n_chan != 3)
    art_warn ("art_render_clear_rgb: called on render with %d channels, only works with 3\n",
	      render->n_chan);
  else
    {
      int r, g, b;

      render->clear = ART_TRUE;
      r = clear_rgb >> 16;
      g = (clear_rgb >> 8) & 0xff;
      b = clear_rgb & 0xff;
      render->clear_color[0] = ART_PIX_MAX_FROM_8(r);
      render->clear_color[1] = ART_PIX_MAX_FROM_8(g);
      render->clear_color[2] = ART_PIX_MAX_FROM_8(b);
    }
}

static void
art_render_nop_done (ArtRenderCallback *self, ArtRender *render)
{
}

static void
art_render_clear_render_rgb8 (ArtRenderCallback *self, ArtRender *render,
			      art_u8 *dest, int y)
{
  int width = render->x1 - render->x0;
  art_u8 r, g, b;
  ArtPixMaxDepth color_max;

  color_max = render->clear_color[0];
  r = ART_PIX_8_FROM_MAX (color_max);
  color_max = render->clear_color[1];
  g = ART_PIX_8_FROM_MAX (color_max);
  color_max = render->clear_color[2];
  b = ART_PIX_8_FROM_MAX (color_max);

  art_rgb_fill_run (dest, r, g, b, width);
}

static void
art_render_clear_render_8 (ArtRenderCallback *self, ArtRender *render,
			   art_u8 *dest, int y)
{
  int width = render->x1 - render->x0;
  int i, j;
  int n_ch = render->n_chan + (render->alpha_type != ART_ALPHA_NONE);
  int ix;
  art_u8 color[ART_MAX_CHAN + 1];

  for (j = 0; j < n_ch; j++)
    {
      ArtPixMaxDepth color_max = render->clear_color[j];
      color[j] = ART_PIX_8_FROM_MAX (color_max);
    }

  ix = 0;
  for (i = 0; i < width; i++)
    for (j = 0; j < n_ch; j++)
      dest[ix++] = color[j];
}

const ArtRenderCallback art_render_clear_rgb8_obj =
{
  art_render_clear_render_rgb8,
  art_render_nop_done
};

const ArtRenderCallback art_render_clear_8_obj =
{
  art_render_clear_render_8,
  art_render_nop_done
};

#if ART_MAX_DEPTH >= 16

static void
art_render_clear_render_16 (ArtRenderCallback *self, ArtRender *render,
			    art_u8 *dest, int y)
{
  int width = render->x1 - render->x0;
  int i, j;
  int n_ch = render->n_chan + (render->alpha_type != ART_ALPHA_NONE);
  int ix;
  art_u16 *dest_16 = (art_u16 *)dest;
  art_u8 color[ART_MAX_CHAN + 1];

  for (j = 0; j < n_ch; j++)
    {
      int color_16 = render->clear_color[j];
      color[j] = color_16;
    }

  ix = 0;
  for (i = 0; i < width; i++)
    for (j = 0; j < n_ch; j++)
      dest_16[ix++] = color[j];
}

const ArtRenderCallback art_render_clear_16_obj =
{
  art_render_clear_render_16,
  art_render_nop_done
};

#endif /* ART_MAX_DEPTH >= 16 */

/* todo: inline */
static ArtRenderCallback *
art_render_choose_clear_callback (ArtRender *render)
{
  ArtRenderCallback *clear_callback;

  if (render->depth == 8)
    {
      if (render->n_chan == 3 &&
	  render->alpha_type == ART_ALPHA_NONE)
	clear_callback = (ArtRenderCallback *)&art_render_clear_rgb8_obj;
      else
	clear_callback = (ArtRenderCallback *)&art_render_clear_8_obj;
    }
#if ART_MAX_DEPTH >= 16
  else if (render->depth == 16)
    clear_callback = (ArtRenderCallback *)&art_render_clear_16_obj;
#endif
  else
    {
      art_die ("art_render_choose_clear_callback: inconsistent render->depth = %d\n",
	       render->depth);
    }
  return clear_callback;
}

#if 0
/* todo: get around to writing this */
static void
art_render_composite_render_noa_8_norm (ArtRenderCallback *self, ArtRender *render,
					art_u8 *dest, int y)
{
  int width = render->x1 - render->x0;

}
#endif

/* This is the most general form of the function. It is slow but
   (hopefully) correct. Actually, I'm still worried about roundoff
   errors in the premul case - it seems to me that an off-by-one could
   lead to overflow. */
static void
art_render_composite (ArtRenderCallback *self, ArtRender *render,
					art_u8 *dest, int y)
{
  ArtRenderMaskRun *run = render->run;
  art_u32 depth = render->depth;
  int n_run = render->n_run;
  int x0 = render->x0;
  int x;
  int run_x0, run_x1;
  art_u8 *alpha_buf = render->alpha_buf;
  art_u8 *image_buf = render->image_buf;
  int i, j;
  art_u32 tmp;
  art_u32 run_alpha;
  art_u32 alpha;
  int image_ix;
  art_u16 src[ART_MAX_CHAN + 1];
  art_u16 dst[ART_MAX_CHAN + 1];
  int n_chan = render->n_chan;
  ArtAlphaType alpha_type = render->alpha_type;
  int n_ch = n_chan + (alpha_type != ART_ALPHA_NONE);
  int dst_pixstride = n_ch * (depth >> 3);
  int buf_depth = render->buf_depth;
  ArtAlphaType buf_alpha = render->buf_alpha;
  int buf_n_ch = n_chan + (buf_alpha != ART_ALPHA_NONE);
  int buf_pixstride = buf_n_ch * (buf_depth >> 3);
  art_u8 *bufptr;
  art_u32 src_alpha;
  art_u32 src_mul;
  art_u8 *dstptr;
  art_u32 dst_alpha;
  art_u32 dst_mul;

  image_ix = 0;
  for (i = 0; i < n_run - 1; i++)
    {
      run_x0 = run[i].x;
      run_x1 = run[i + 1].x;
      tmp = run[i].alpha;
      if (tmp < 0x8100)
	continue;

      run_alpha = (tmp + (tmp >> 8) + (tmp >> 16) - 0x8000) >> 8; /* range [0 .. 0x10000] */
      bufptr = image_buf + (run_x0 - x0) * buf_pixstride;
      dstptr = dest + (run_x0 - x0) * dst_pixstride;
      for (x = run_x0; x < run_x1; x++)
	{
	  if (alpha_buf)
	    {
	      if (depth == 8)
		{
		  tmp = run_alpha * alpha_buf[x - x0] + 0x80;
		  /* range 0x80 .. 0xff0080 */
		  alpha = (tmp + (tmp >> 8) + (tmp >> 16)) >> 8;
		}
	      else /* (depth == 16) */
		{
		  tmp = ((art_u16 *)alpha_buf)[x - x0];
		  tmp = (run_alpha * tmp + 0x8000) >> 8;
		  /* range 0x80 .. 0xffff80 */
		  alpha = (tmp + (tmp >> 16)) >> 8;
		}
	    }
	  else
	    alpha = run_alpha;
	  /* alpha is run_alpha * alpha_buf[x], range 0 .. 0x10000 */

	  /* convert (src pixel * alpha) to premul alpha form,
	     store in src as 0..0xffff range */
	  if (buf_alpha == ART_ALPHA_NONE)
	    {
	      src_alpha = alpha;
	      src_mul = src_alpha;
	    }
	  else
	    {
	      if (buf_depth == 8)
		{
		  tmp = alpha * bufptr[n_chan] + 0x80;
		  /* range 0x80 .. 0xff0080 */
		  src_alpha = (tmp + (tmp >> 8) + (tmp >> 16)) >> 8;
		}
	      else /* (depth == 16) */
		{
		  tmp = ((art_u16 *)bufptr)[n_chan];
		  tmp = (alpha * tmp + 0x8000) >> 8;
		  /* range 0x80 .. 0xffff80 */
		  src_alpha = (tmp + (tmp >> 16)) >> 8;
		}
	      if (buf_alpha == ART_ALPHA_SEPARATE)
		src_mul = src_alpha;
	      else /* buf_alpha == (ART_ALPHA_PREMUL) */
		src_mul = alpha;
	    }
	  /* src_alpha is the (alpha of the source pixel * alpha),
	     range 0..0x10000 */

	  if (buf_depth == 8)
	    {
	      src_mul *= 0x101;
	      for (j = 0; j < n_chan; j++)
		src[j] = (bufptr[j] * src_mul + 0x8000) >> 16;
	    }
	  else if (buf_depth == 16)
	    {
	      for (j = 0; j < n_chan; j++)
		src[j] = (((art_u16 *)bufptr)[j] * src_mul + 0x8000) >> 16;
	    }
	  bufptr += buf_pixstride;

	  /* src[0..n_chan - 1] (range 0..0xffff) and src_alpha (range
             0..0x10000) now contain the source pixel with
             premultiplied alpha */

	  /* convert dst pixel to premul alpha form,
	     store in dst as 0..0xffff range */
	  if (alpha_type == ART_ALPHA_NONE)
	    {
	      dst_alpha = 0x10000;
	      dst_mul = dst_alpha;
	    }
	  else
	    {
	      if (depth == 8)
		{
		  tmp = dstptr[n_chan];
		  /* range 0..0xff */
		  dst_alpha = (tmp << 8) + tmp + (tmp >> 7);
		}
	      else /* (depth == 16) */
		{
		  tmp = ((art_u16 *)bufptr)[n_chan];
		  dst_alpha = (tmp + (tmp >> 15));
		}
	      if (alpha_type == ART_ALPHA_SEPARATE)
		dst_mul = dst_alpha;
	      else /* (alpha_type == ART_ALPHA_PREMUL) */
		dst_mul = 0x10000;
	    }
	  /* dst_alpha is the alpha of the dest pixel,
	     range 0..0x10000 */

	  if (depth == 8)
	    {
	      dst_mul *= 0x101;
	      for (j = 0; j < n_chan; j++)
		dst[j] = (dstptr[j] * dst_mul + 0x8000) >> 16;
	    }
	  else if (buf_depth == 16)
	    {
	      for (j = 0; j < n_chan; j++)
		dst[j] = (((art_u16 *)dstptr)[j] * dst_mul + 0x8000) >> 16;
	    }

	  /* do the compositing, dst = (src over dst) */
	  for (j = 0; j < n_chan; j++)
	    {
	      art_u32 srcv, dstv;
	      art_u32 tmp;

	      srcv = src[j];
	      dstv = dst[j];
	      tmp = ((dstv * (0x10000 - src_alpha) + 0x8000) >> 16) + srcv;
	      tmp -= tmp >> 16;
	      dst[j] = tmp;
	    }

	  if (alpha_type == ART_ALPHA_NONE)
	    {
	      if (depth == 8)
		dst_mul = 0xff;
	      else /* (depth == 16) */
		dst_mul = 0xffff;
	    }
	  else
	    {
	      if (src_alpha >= 0x10000)
		dst_alpha = 0x10000;
	      else
		dst_alpha += ((((0x10000 - dst_alpha) * src_alpha) >> 8) + 0x80) >> 8;
	      if (alpha_type == ART_ALPHA_PREMUL || dst_alpha == 0)
		{
		  if (depth == 8)
		    dst_mul = 0xff;
		  else /* (depth == 16) */
		    dst_mul = 0xffff;
		}
	      else /* (ALPHA_TYPE == ART_ALPHA_SEPARATE && dst_alpha != 0) */
		{
		  if (depth == 8)
		    dst_mul = 0xff0000 / dst_alpha;
		  else /* (depth == 16) */
		    dst_mul = 0xffff0000 / dst_alpha;
		}
	    }
	  if (depth == 8)
	    {
	      for (j = 0; j < n_chan; j++)
		dstptr[j] = (dst[j] * dst_mul + 0x8000) >> 16;
	      if (alpha_type != ART_ALPHA_NONE)
		dstptr[n_chan] = (dst_alpha * 0xff + 0x8000) >> 16;
	    }
	  else if (depth == 16)
	    {
	      for (j = 0; j < n_chan; j++)
		((art_u16 *)dstptr)[j] = (dst[j] * dst_mul + 0x8000) >> 16;
	      if (alpha_type != ART_ALPHA_NONE)
		dstptr[n_chan] = (dst_alpha * 0xffff + 0x8000) >> 16;
	    }
	  dstptr += dst_pixstride;
	}
    }
}

const ArtRenderCallback art_render_composite_obj =
{
  art_render_composite,
  art_render_nop_done
};

static void
art_render_composite_8 (ArtRenderCallback *self, ArtRender *render,
			art_u8 *dest, int y)
{
  ArtRenderMaskRun *run = render->run;
  int n_run = render->n_run;
  int x0 = render->x0;
  int x;
  int run_x0, run_x1;
  art_u8 *alpha_buf = render->alpha_buf;
  art_u8 *image_buf = render->image_buf;
  int i, j;
  art_u32 tmp;
  art_u32 run_alpha;
  art_u32 alpha;
  int image_ix;
  int n_chan = render->n_chan;
  ArtAlphaType alpha_type = render->alpha_type;
  int n_ch = n_chan + (alpha_type != ART_ALPHA_NONE);
  int dst_pixstride = n_ch;
  ArtAlphaType buf_alpha = render->buf_alpha;
  int buf_n_ch = n_chan + (buf_alpha != ART_ALPHA_NONE);
  int buf_pixstride = buf_n_ch;
  art_u8 *bufptr;
  art_u32 src_alpha;
  art_u32 src_mul;
  art_u8 *dstptr;
  art_u32 dst_alpha;
  art_u32 dst_mul, dst_save_mul;

  image_ix = 0;
  for (i = 0; i < n_run - 1; i++)
    {
      run_x0 = run[i].x;
      run_x1 = run[i + 1].x;
      tmp = run[i].alpha;
      if (tmp < 0x10000)
	continue;

      run_alpha = (tmp + (tmp >> 8) + (tmp >> 16) - 0x8000) >> 8; /* range [0 .. 0x10000] */
      bufptr = image_buf + (run_x0 - x0) * buf_pixstride;
      dstptr = dest + (run_x0 - x0) * dst_pixstride;
      for (x = run_x0; x < run_x1; x++)
	{
	  if (alpha_buf)
	    {
	      tmp = run_alpha * alpha_buf[x - x0] + 0x80;
	      /* range 0x80 .. 0xff0080 */
	      alpha = (tmp + (tmp >> 8) + (tmp >> 16)) >> 8;
	    }
	  else
	    alpha = run_alpha;
	  /* alpha is run_alpha * alpha_buf[x], range 0 .. 0x10000 */

	  /* convert (src pixel * alpha) to premul alpha form,
	     store in src as 0..0xffff range */
	  if (buf_alpha == ART_ALPHA_NONE)
	    {
	      src_alpha = alpha;
	      src_mul = src_alpha;
	    }
	  else
	    {
	      tmp = alpha * bufptr[n_chan] + 0x80;
	      /* range 0x80 .. 0xff0080 */
	      src_alpha = (tmp + (tmp >> 8) + (tmp >> 16)) >> 8;

	      if (buf_alpha == ART_ALPHA_SEPARATE)
		src_mul = src_alpha;
	      else /* buf_alpha == (ART_ALPHA_PREMUL) */
		src_mul = alpha;
	    }
	  /* src_alpha is the (alpha of the source pixel * alpha),
	     range 0..0x10000 */

	  src_mul *= 0x101;

	  if (alpha_type == ART_ALPHA_NONE)
	    {
	      dst_alpha = 0x10000;
	      dst_mul = dst_alpha;
	    }
	  else
	    {
	      tmp = dstptr[n_chan];
	      /* range 0..0xff */
	      dst_alpha = (tmp << 8) + tmp + (tmp >> 7);
	      if (alpha_type == ART_ALPHA_SEPARATE)
		dst_mul = dst_alpha;
	      else /* (alpha_type == ART_ALPHA_PREMUL) */
		dst_mul = 0x10000;
	    }
	  /* dst_alpha is the alpha of the dest pixel,
	     range 0..0x10000 */

	  dst_mul *= 0x101;

	  if (alpha_type == ART_ALPHA_NONE)
	    {
	      dst_save_mul = 0xff;
	    }
	  else
	    {
	      if (src_alpha >= 0x10000)
		dst_alpha = 0x10000;
	      else
		dst_alpha += ((((0x10000 - dst_alpha) * src_alpha) >> 8) + 0x80) >> 8;
	      if (alpha_type == ART_ALPHA_PREMUL || dst_alpha == 0)
		{
		  dst_save_mul = 0xff;
		}
	      else /* (ALPHA_TYPE == ART_ALPHA_SEPARATE && dst_alpha != 0) */
		{
		  dst_save_mul = 0xff0000 / dst_alpha;
		}
	    }
	  for (j = 0; j < n_chan; j++)
	    {
	      art_u32 src, dst;
	      art_u32 tmp;

	      src = (bufptr[j] * src_mul + 0x8000) >> 16;
	      dst = (dstptr[j] * dst_mul + 0x8000) >> 16;
	      tmp = ((dst * (0x10000 - src_alpha) + 0x8000) >> 16) + src;
	      tmp -= tmp >> 16;
	      dstptr[j] = (tmp * dst_save_mul + 0x8000) >> 16;
	    }
	  if (alpha_type != ART_ALPHA_NONE)
	    dstptr[n_chan] = (dst_alpha * 0xff + 0x8000) >> 16;

	  bufptr += buf_pixstride;
	  dstptr += dst_pixstride;
	}
    }
}

const ArtRenderCallback art_render_composite_8_obj =
{
  art_render_composite_8,
  art_render_nop_done
};

/* todo: inline */
static ArtRenderCallback *
art_render_choose_compositing_callback (ArtRender *render)
{
  if (render->depth == 8 && render->buf_depth == 8)
    return (ArtRenderCallback *)&art_render_composite_8_obj;
  return (ArtRenderCallback *)&art_render_composite_obj;
}

/**
 * art_render_invoke_callbacks: Invoke the callbacks in the render object.
 * @render: The render object.
 * @y: The current Y coordinate value.
 *
 * Invokes the callbacks of the render object in the appropriate
 * order.  Drivers should call this routine once per scanline.
 *
 * todo: should management of dest devolve to this routine? very
 * plausibly yes.
 **/
void
art_render_invoke_callbacks (ArtRender *render, art_u8 *dest, int y)
{
  ArtRenderPriv *priv = (ArtRenderPriv *)render;
  int i;

  for (i = 0; i < priv->n_callbacks; i++)
    {
      ArtRenderCallback *callback;

      callback = priv->callbacks[i];
      callback->render (callback, render, dest, y);
    }
}

/**
 * art_render_invoke: Perform the requested rendering task.
 * @render: The render object.
 *
 * Invokes the renderer and all sources associated with it, to perform
 * the requested rendering task.
 **/
void
art_render_invoke (ArtRender *render)
{
  ArtRenderPriv *priv = (ArtRenderPriv *)render;
  int width;
  int best_driver, best_score;
  int i;
  int n_callbacks, n_callbacks_max;
  ArtImageSource *image_source;
  ArtImageSourceFlags image_flags;
  int buf_depth;
  ArtAlphaType buf_alpha;
  art_boolean first = ART_TRUE;

  if (render == NULL)
    {
      art_warn ("art_render_invoke: called with render == NULL\n");
      return;
    }
  if (priv->image_source == NULL)
    {
      art_warn ("art_render_invoke: no image source given\n");
      return;
    }

  width = render->x1 - render->x0;

  render->run = art_new (ArtRenderMaskRun, width + 1);

  /* Elect a mask source as driver. */
  best_driver = -1;
  best_score = 0;
  for (i = 0; i < priv->n_mask_source; i++)
    {
      int score;
      ArtMaskSource *mask_source;

      mask_source = priv->mask_source[i];
      score = mask_source->can_drive (mask_source, render);
      if (score > best_score)
	{
	  best_score = score;
	  best_driver = i;
	}
    }

  /* Allocate alpha buffer if needed. */
  if (priv->n_mask_source > 1 ||
      (priv->n_mask_source == 1 && best_driver < 0))
    {
      render->alpha_buf = art_new (art_u8, (width * render->depth) >> 3);
    }

  /* Negotiate image rendering and compositing. */
  image_source = priv->image_source;
  image_source->negotiate (image_source, render, &image_flags, &buf_depth,
			   &buf_alpha);

  /* Build callback list. */
  n_callbacks_max = priv->n_mask_source + 3;
  priv->callbacks = art_new (ArtRenderCallback *, n_callbacks_max);
  n_callbacks = 0;
  for (i = 0; i < priv->n_mask_source; i++)
    if (i != best_driver)
      {
	ArtMaskSource *mask_source = priv->mask_source[i];

	mask_source->prepare (mask_source, render, first);
	first = ART_FALSE;
	priv->callbacks[n_callbacks++] = &mask_source->super;
      }

  if (render->clear && !(image_flags & ART_IMAGE_SOURCE_CAN_CLEAR))
    priv->callbacks[n_callbacks++] =
      art_render_choose_clear_callback (render);

  priv->callbacks[n_callbacks++] = &image_source->super;

  /* Allocate image buffer and add compositing callback if needed. */
  if (!(image_flags & ART_IMAGE_SOURCE_CAN_COMPOSITE))
    {
      int bytespp = ((render->n_chan + (buf_alpha != ART_ALPHA_NONE)) *
		     buf_depth) >> 3;
      render->buf_depth = buf_depth;
      render->buf_alpha = buf_alpha;
      render->image_buf = art_new (art_u8, width * bytespp);
      priv->callbacks[n_callbacks++] =
	art_render_choose_compositing_callback (render);
    }

  priv->n_callbacks = n_callbacks;

  if (render->need_span)
    render->span_x = art_new (int, width + 1);

  /* Invoke the driver */
  if (best_driver >= 0)
    {
      ArtMaskSource *driver;

      driver = priv->mask_source[best_driver];
      driver->invoke_driver (driver, render);
    }
  else
    {
      art_u8 *dest_ptr = render->pixels;
      int y;

      /* Dummy driver */
      render->n_run = 2;
      render->run[0].x = render->x0;
      render->run[0].alpha = 0x8000 + 0xff * render->opacity;
      render->run[1].x = render->x1;
      render->run[1].alpha = 0x8000;
      if (render->need_span)
	{
	  render->n_span = 2;
	  render->span_x[0] = render->x0;
	  render->span_x[1] = render->x1;
	}
      for (y = render->y0; y < render->y1; y++)
	{
	  art_render_invoke_callbacks (render, dest_ptr, y);
	  dest_ptr += render->rowstride;
	}
    }

  if (priv->mask_source != NULL)
    art_free (priv->mask_source);

  /* clean up callbacks */
  for (i = 0; i < priv->n_callbacks; i++)
    {
      ArtRenderCallback *callback;

      callback = priv->callbacks[i];
      callback->done (callback, render);
    }

  /* Tear down object */
  if (render->alpha_buf != NULL)
    art_free (render->alpha_buf);
  if (render->image_buf != NULL)
    art_free (render->image_buf);
  art_free (render->run);
  if (render->span_x != NULL)
    art_free (render->span_x);
  art_free (priv->callbacks);
  art_free (render);
}

/**
 * art_render_mask_solid: Add a solid translucent mask.
 * @render: The render object.
 * @opacity: Opacity in [0..0x10000] form.
 *
 * Adds a translucent mask to the rendering object.
 **/
void
art_render_mask_solid (ArtRender *render, int opacity)
{
  art_u32 old_opacity = render->opacity;
  art_u32 new_opacity_tmp;

  if (opacity == 0x10000)
    /* avoid potential overflow */
    return;
  new_opacity_tmp = old_opacity * (art_u32)opacity + 0x8000;
  render->opacity = new_opacity_tmp >> 16;
}

/**
 * art_render_add_mask_source: Add a mask source to the render object.
 * @render: Render object.
 * @mask_source: Mask source to add.
 *
 * This routine adds a mask source to the render object. In general,
 * client api's for adding mask sources should just take a render object,
 * then the mask source creation function should call this function.
 * Clients should never have to call this function directly, unless of
 * course they're creating custom mask sources.
 **/
void
art_render_add_mask_source (ArtRender *render, ArtMaskSource *mask_source)
{
  ArtRenderPriv *priv = (ArtRenderPriv *)render;
  int n_mask_source = priv->n_mask_source++;

  if (n_mask_source == 0)
    priv->mask_source = art_new (ArtMaskSource *, 1);
  /* This predicate is true iff n_mask_source is a power of two */
  else if (!(n_mask_source & (n_mask_source - 1)))
    priv->mask_source = art_renew (priv->mask_source, ArtMaskSource *,
				   n_mask_source << 1);

  priv->mask_source[n_mask_source] = mask_source;
}

/**
 * art_render_add_image_source: Add a mask source to the render object.
 * @render: Render object.
 * @image_source: Image source to add.
 *
 * This routine adds an image source to the render object. In general,
 * client api's for adding image sources should just take a render
 * object, then the mask source creation function should call this
 * function.  Clients should never have to call this function
 * directly, unless of course they're creating custom image sources.
 **/
void
art_render_add_image_source (ArtRender *render, ArtImageSource *image_source)
{
  ArtRenderPriv *priv = (ArtRenderPriv *)render;

  if (priv->image_source != NULL)
    {
      art_warn ("art_render_add_image_source: image source already present.\n");
      return;
    }
  priv->image_source = image_source;
}

/* Solid image source object and methods. Perhaps this should go into a
   separate file. */

typedef struct _ArtImageSourceSolid ArtImageSourceSolid;

struct _ArtImageSourceSolid {
  ArtImageSource super;
  ArtPixMaxDepth color[ART_MAX_CHAN];
  art_u32 *rgbtab;
  art_boolean init;
};

static void
art_render_image_solid_done (ArtRenderCallback *self, ArtRender *render)
{
  ArtImageSourceSolid *z = (ArtImageSourceSolid *)self;

  if (z->rgbtab != NULL)
    art_free (z->rgbtab);
  art_free (self);
}

static void
art_render_image_solid_rgb8_opaq_init (ArtImageSourceSolid *self, ArtRender *render)
{
  ArtImageSourceSolid *z = (ArtImageSourceSolid *)self;
  ArtPixMaxDepth color_max;
  int r_fg, g_fg, b_fg;
  int r_bg, g_bg, b_bg;
  int r, g, b;
  int dr, dg, db;
  int i;
  int tmp;
  art_u32 *rgbtab;

  rgbtab = art_new (art_u32, 256);
  z->rgbtab = rgbtab;

  color_max = self->color[0];
  r_fg = ART_PIX_8_FROM_MAX (color_max);
  color_max = self->color[1];
  g_fg = ART_PIX_8_FROM_MAX (color_max);
  color_max = self->color[2];
  b_fg = ART_PIX_8_FROM_MAX (color_max);

  color_max = render->clear_color[0];
  r_bg = ART_PIX_8_FROM_MAX (color_max);
  color_max = render->clear_color[1];
  g_bg = ART_PIX_8_FROM_MAX (color_max);
  color_max = render->clear_color[2];
  b_bg = ART_PIX_8_FROM_MAX (color_max);

  r = (r_bg << 16) + 0x8000;
  g = (g_bg << 16) + 0x8000;
  b = (b_bg << 16) + 0x8000;
  tmp = ((r_fg - r_bg) << 16) + 0x80;
  dr = (tmp + (tmp >> 8)) >> 8;
  tmp = ((g_fg - g_bg) << 16) + 0x80;
  dg = (tmp + (tmp >> 8)) >> 8;
  tmp = ((b_fg - b_bg) << 16) + 0x80;
  db = (tmp + (tmp >> 8)) >> 8;

  for (i = 0; i < 256; i++)
    {
      rgbtab[i] = (r & 0xff0000) | ((g & 0xff0000) >> 8) | (b >> 16);
      r += dr;
      g += dg;
      b += db;
    }
}

static void
art_render_image_solid_rgb8_opaq (ArtRenderCallback *self, ArtRender *render,
				  art_u8 *dest, int y)
{
  ArtImageSourceSolid *z = (ArtImageSourceSolid *)self;
  ArtRenderMaskRun *run = render->run;
  int n_run = render->n_run;
  art_u32 *rgbtab = z->rgbtab;
  art_u32 rgb;
  int x0 = render->x0;
  int x1 = render->x1;
  int run_x0, run_x1;
  int i;
  int ix;

  if (n_run > 0)
    {
      run_x1 = run[0].x;
      if (run_x1 > x0)
	{
	  rgb = rgbtab[0];
	  art_rgb_fill_run (dest,
			    rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff,
			    run_x1 - x0);
	}
      for (i = 0; i < n_run - 1; i++)
	{
	  run_x0 = run_x1;
	  run_x1 = run[i + 1].x;
	  rgb = rgbtab[(run[i].alpha >> 16) & 0xff];
	  ix = (run_x0 - x0) * 3;
#define OPTIMIZE_LEN_1
#ifdef OPTIMIZE_LEN_1
	  if (run_x1 - run_x0 == 1)
	    {
	      dest[ix] = rgb >> 16;
	      dest[ix + 1] = (rgb >> 8) & 0xff;
	      dest[ix + 2] = rgb & 0xff;
	    }
	  else
	    {
	      art_rgb_fill_run (dest + ix,
				rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff,
				run_x1 - run_x0);
	    }
#else
	  art_rgb_fill_run (dest + ix,
			    rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff,
			    run_x1 - run_x0);
#endif
	}
    }
  else
    {
      run_x1 = x0;
    }
  if (run_x1 < x1)
    {
      rgb = rgbtab[0];
      art_rgb_fill_run (dest + (run_x1 - x0) * 3,
			rgb >> 16, (rgb >> 8) & 0xff, rgb & 0xff,
			x1 - run_x1);
    }
}

static void
art_render_image_solid_rgb8 (ArtRenderCallback *self, ArtRender *render,
			     art_u8 *dest, int y)
{
  ArtImageSourceSolid *z = (ArtImageSourceSolid *)self;
  int width = render->x1 - render->x0;
  art_u8 r, g, b;
  ArtPixMaxDepth color_max;

  /* todo: replace this simple test with real sparseness */
  if (z->init)
    return;
  z->init = ART_TRUE;

  color_max = z->color[0];
  r = ART_PIX_8_FROM_MAX (color_max);
  color_max = z->color[1];
  g = ART_PIX_8_FROM_MAX (color_max);
  color_max = z->color[2];
  b = ART_PIX_8_FROM_MAX (color_max);

  art_rgb_fill_run (render->image_buf, r, g, b, width);
}

static void
art_render_image_solid_negotiate (ArtImageSource *self, ArtRender *render,
				  ArtImageSourceFlags *p_flags,
				  int *p_buf_depth, ArtAlphaType *p_alpha)
{
  ArtImageSourceSolid *z = (ArtImageSourceSolid *)self;
  ArtImageSourceFlags flags = 0;
  static void (*render_cbk) (ArtRenderCallback *self, ArtRender *render,
			     art_u8 *dest, int y);

  render_cbk = NULL;

  if (render->depth == 8 && render->n_chan == 3 &&
      render->alpha_type == ART_ALPHA_NONE)
    {
      if (render->clear)
	{
	  render_cbk = art_render_image_solid_rgb8_opaq;
	  flags |= ART_IMAGE_SOURCE_CAN_CLEAR;
	  art_render_image_solid_rgb8_opaq_init (z, render);
	}
      flags |= ART_IMAGE_SOURCE_CAN_COMPOSITE;
    }
  if (render_cbk == NULL)
    {
      if (render->depth == 8)
	{
	  render_cbk = art_render_image_solid_rgb8;
	  *p_buf_depth = 8;
	  *p_alpha = ART_ALPHA_NONE; /* todo */
	}
    }
  /* todo: general case */
  self->super.render = render_cbk;
  *p_flags = flags;
}

/**
 * art_render_image_solid: Add a solid color image source.
 * @render: The render object.
 * @color: Color.
 *
 * Adds an image source with the solid color given by @color. The
 * color need not be retained in memory after this call.
 **/
void
art_render_image_solid (ArtRender *render, ArtPixMaxDepth *color)
{
  ArtImageSourceSolid *image_source;
  int i;

  image_source = art_new (ArtImageSourceSolid, 1);
  image_source->super.super.render = NULL;
  image_source->super.super.done = art_render_image_solid_done;
  image_source->super.negotiate = art_render_image_solid_negotiate;

  for (i = 0; i < render->n_chan; i++)
    image_source->color[i] = color[i];

  image_source->rgbtab = NULL;
  image_source->init = ART_FALSE;

  art_render_add_image_source (render, &image_source->super);
}
