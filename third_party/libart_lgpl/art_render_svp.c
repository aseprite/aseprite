/*
 * art_render_gradient.c: SVP mask source for modular rendering.
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
 *
 * Authors: Raph Levien <raph@acm.org>
 */

#include "art_misc.h"
#include "art_alphagamma.h"
#include "art_svp.h"
#include "art_svp_render_aa.h"

#include "art_render.h"
#include "art_render_svp.h"

typedef struct _ArtMaskSourceSVP ArtMaskSourceSVP;

struct _ArtMaskSourceSVP {
  ArtMaskSource super;
  ArtRender *render;
  const ArtSVP *svp;
  art_u8 *dest_ptr;
};

static void
art_render_svp_done (ArtRenderCallback *self, ArtRender *render)
{
  art_free (self);
}

static int
art_render_svp_can_drive (ArtMaskSource *self, ArtRender *render)
{
  return 10;
}

/* The basic art_render_svp_callback function is repeated four times,
   for all combinations of non-unit opacity and generating spans. In
   general, I'd consider this bad style, but in this case I plead
   a measurable performance improvement. */

static void
art_render_svp_callback (void *callback_data, int y,
			 int start, ArtSVPRenderAAStep *steps, int n_steps)
{
  ArtMaskSourceSVP *z = (ArtMaskSourceSVP *)callback_data;
  ArtRender *render = z->render;
  int n_run = 0;
  int i;
  int running_sum = start;
  int x0 = render->x0;
  int x1 = render->x1;
  int run_x0, run_x1;
  ArtRenderMaskRun *run = render->run;

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      if (run_x1 > x0 && running_sum > 0x80ff)
	{
	  run[0].x = x0;
	  run[0].alpha = running_sum;
	  n_run++;
	}

      for (i = 0; i < n_steps - 1; i++)
	{
          running_sum += steps[i].delta;
          run_x0 = run_x1;
          run_x1 = steps[i + 1].x;
	  if (run_x1 > run_x0)
	    {
	      run[n_run].x = run_x0;
	      run[n_run].alpha = running_sum;
	      n_run++;
	    }
	}
      if (x1 > run_x1)
	{
	  running_sum += steps[n_steps - 1].delta;
	  run[n_run].x = run_x1;
	  run[n_run].alpha = running_sum;
	  n_run++;
	}
      if (running_sum > 0x80ff)
	{
	  run[n_run].x = x1;
	  run[n_run].alpha = 0x8000;
	  n_run++;
	}
    }

  render->n_run = n_run;

  art_render_invoke_callbacks (render, z->dest_ptr, y);

  z->dest_ptr += render->rowstride;
}

static void
art_render_svp_callback_span (void *callback_data, int y,
			      int start, ArtSVPRenderAAStep *steps, int n_steps)
{
  ArtMaskSourceSVP *z = (ArtMaskSourceSVP *)callback_data;
  ArtRender *render = z->render;
  int n_run = 0;
  int n_span = 0;
  int i;
  int running_sum = start;
  int x0 = render->x0;
  int x1 = render->x1;
  int run_x0, run_x1;
  ArtRenderMaskRun *run = render->run;
  int *span_x = render->span_x;

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      if (run_x1 > x0 && running_sum > 0x80ff)
	{
	  run[0].x = x0;
	  run[0].alpha = running_sum;
	  n_run++;
	  span_x[0] = x0;
	  n_span++;
	}

      for (i = 0; i < n_steps - 1; i++)
	{
          running_sum += steps[i].delta;
          run_x0 = run_x1;
          run_x1 = steps[i + 1].x;
	  if (run_x1 > run_x0)
	    {
	      run[n_run].x = run_x0;
	      run[n_run].alpha = running_sum;
	      n_run++;
	      if ((n_span & 1) != (running_sum > 0x80ff))
		span_x[n_span++] = run_x0;
	    }
	}
      if (x1 > run_x1)
	{
	  running_sum += steps[n_steps - 1].delta;
	  run[n_run].x = run_x1;
	  run[n_run].alpha = running_sum;
	  n_run++;
	  if ((n_span & 1) != (running_sum > 0x80ff))
	    span_x[n_span++] = run_x1;
	}
      if (running_sum > 0x80ff)
	{
	  run[n_run].x = x1;
	  run[n_run].alpha = 0x8000;
	  n_run++;
	  span_x[n_span++] = x1;
	}
    }

  render->n_run = n_run;
  render->n_span = n_span;

  art_render_invoke_callbacks (render, z->dest_ptr, y);

  z->dest_ptr += render->rowstride;
}

static void
art_render_svp_callback_opacity (void *callback_data, int y,
				 int start, ArtSVPRenderAAStep *steps, int n_steps)
{
  ArtMaskSourceSVP *z = (ArtMaskSourceSVP *)callback_data;
  ArtRender *render = z->render;
  int n_run = 0;
  int i;
  art_u32 running_sum;
  int x0 = render->x0;
  int x1 = render->x1;
  int run_x0, run_x1;
  ArtRenderMaskRun *run = render->run;
  art_u32 opacity = render->opacity;
  art_u32 alpha;

  running_sum = start - 0x7f80;

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      alpha = ((running_sum >> 8) * opacity + 0x80080) >> 8;
      if (run_x1 > x0 && alpha > 0x80ff)
	{
	  run[0].x = x0;
	  run[0].alpha = alpha;
	  n_run++;
	}

      for (i = 0; i < n_steps - 1; i++)
	{
          running_sum += steps[i].delta;
          run_x0 = run_x1;
          run_x1 = steps[i + 1].x;
	  if (run_x1 > run_x0)
	    {
	      run[n_run].x = run_x0;
	      alpha = ((running_sum >> 8) * opacity + 0x80080) >> 8;
	      run[n_run].alpha = alpha;
	      n_run++;
	    }
	}
      if (x1 > run_x1)
	{
	  running_sum += steps[n_steps - 1].delta;
	  run[n_run].x = run_x1;
	  alpha = ((running_sum >> 8) * opacity + 0x80080) >> 8;
	  run[n_run].alpha = alpha;
	  n_run++;
	}
      if (alpha > 0x80ff)
	{
	  run[n_run].x = x1;
	  run[n_run].alpha = 0x8000;
	  n_run++;
	}
    }

  render->n_run = n_run;

  art_render_invoke_callbacks (render, z->dest_ptr, y);

  z->dest_ptr += render->rowstride;
}

static void
art_render_svp_callback_opacity_span (void *callback_data, int y,
				      int start, ArtSVPRenderAAStep *steps, int n_steps)
{
  ArtMaskSourceSVP *z = (ArtMaskSourceSVP *)callback_data;
  ArtRender *render = z->render;
  int n_run = 0;
  int n_span = 0;
  int i;
  art_u32 running_sum;
  int x0 = render->x0;
  int x1 = render->x1;
  int run_x0, run_x1;
  ArtRenderMaskRun *run = render->run;
  int *span_x = render->span_x;
  art_u32 opacity = render->opacity;
  art_u32 alpha;

  running_sum = start - 0x7f80;

  if (n_steps > 0)
    {
      run_x1 = steps[0].x;
      alpha = ((running_sum >> 8) * opacity + 0x800080) >> 8;
      if (run_x1 > x0 && alpha > 0x80ff)
	{
	  run[0].x = x0;
	  run[0].alpha = alpha;
	  n_run++;
	  span_x[0] = x0;
	  n_span++;
	}

      for (i = 0; i < n_steps - 1; i++)
	{
          running_sum += steps[i].delta;
          run_x0 = run_x1;
          run_x1 = steps[i + 1].x;
	  if (run_x1 > run_x0)
	    {
	      run[n_run].x = run_x0;
	      alpha = ((running_sum >> 8) * opacity + 0x800080) >> 8;
	      run[n_run].alpha = alpha;
	      n_run++;
	      if ((n_span & 1) != (alpha > 0x80ff))
		span_x[n_span++] = run_x0;
	    }
	}
      if (x1 > run_x1)
	{
	  running_sum += steps[n_steps - 1].delta;
	  run[n_run].x = run_x1;
	  alpha = ((running_sum >> 8) * opacity + 0x800080) >> 8;
	  run[n_run].alpha = alpha;
	  n_run++;
	  if ((n_span & 1) != (alpha > 0x80ff))
	    span_x[n_span++] = run_x1;
	}
      if (alpha > 0x80ff)
	{
	  run[n_run].x = x1;
	  run[n_run].alpha = 0x8000;
	  n_run++;
	  span_x[n_span++] = x1;
	}
    }

  render->n_run = n_run;
  render->n_span = n_span;

  art_render_invoke_callbacks (render, z->dest_ptr, y);

  z->dest_ptr += render->rowstride;
}

static void
art_render_svp_invoke_driver (ArtMaskSource *self, ArtRender *render)
{
  ArtMaskSourceSVP *z = (ArtMaskSourceSVP *)self;
  void (*callback) (void *callback_data,
		    int y,
		    int start,
		    ArtSVPRenderAAStep *steps, int n_steps);

  z->dest_ptr = render->pixels;
  if (render->opacity == 0x10000)
    {
      if (render->need_span)
	callback = art_render_svp_callback_span;
      else
	callback = art_render_svp_callback;
    }
  else
    {
      if (render->need_span)
	callback = art_render_svp_callback_opacity_span;
      else
	callback = art_render_svp_callback_opacity;
    }

  art_svp_render_aa (z->svp,
		     render->x0, render->y0,
		     render->x1, render->y1, callback,
		     self);
  art_render_svp_done (&self->super, render);
}

static void
art_render_svp_prepare (ArtMaskSource *self, ArtRender *render,
			art_boolean first)
{
  /* todo */
  art_die ("art_render_svp non-driver mode not yet implemented.\n");
}

/**
 * art_render_svp: Use an SVP as a render mask source.
 * @render: Render object.
 * @svp: SVP.
 *
 * Adds @svp to the render object as a mask. Note: @svp must remain
 * allocated until art_render_invoke() is called on @render.
 **/
void
art_render_svp (ArtRender *render, const ArtSVP *svp)
{
  ArtMaskSourceSVP *mask_source;
  mask_source = art_new (ArtMaskSourceSVP, 1);

  mask_source->super.super.render = NULL;
  mask_source->super.super.done = art_render_svp_done;
  mask_source->super.can_drive = art_render_svp_can_drive;
  mask_source->super.invoke_driver = art_render_svp_invoke_driver;
  mask_source->super.prepare = art_render_svp_prepare;
  mask_source->render = render;
  mask_source->svp = svp;

  art_render_add_mask_source (render, &mask_source->super);
}
