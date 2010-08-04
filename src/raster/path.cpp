/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <string.h>

#include "raster/blend.h"
#include "raster/image.h"
#include "raster/path.h"

#include "libart_lgpl/libart.h"

static int path_get_element(Path* path, double x, double y);
static void draw_path(Path* path, Image *image, int color, double brush_size, int fill);

//////////////////////////////////////////////////////////////////////

Path::Path(const char* name)
  : GfxObj(GFXOBJ_PATH)
{
  this->name = name ? jstrdup(name) : NULL;
  this->join = PATH_JOIN_ROUND;
  this->cap = PATH_CAP_ROUND;
  this->size = 0;
  this->end = 0;
  this->bpath = NULL;

  path_get_element(this, 0, 0);
  this->bpath[--this->end].code = ART_END;
}

Path::Path(const Path& path)
 : GfxObj(path)
{
  this->join = path.join;
  this->cap = path.cap;
  this->size = path.size;
  this->end = path.end;

  this->bpath = art_new(ArtBpath, this->size);
  for (int c=0; c<this->size; c++) {
    this->bpath[c].code = path.bpath[c].code;
    this->bpath[c].x1 = path.bpath[c].x1;
    this->bpath[c].y1 = path.bpath[c].y1;
    this->bpath[c].x2 = path.bpath[c].x2;
    this->bpath[c].y2 = path.bpath[c].y2;
    this->bpath[c].x3 = path.bpath[c].x3;
    this->bpath[c].y3 = path.bpath[c].y3;
  }
}

Path::~Path()
{
  if (this->bpath)
    art_free(this->bpath);
}

//////////////////////////////////////////////////////////////////////

Path* path_new(const char* name)
{
  return new Path(name);
}

Path* path_new_copy(const Path* path)
{
  ASSERT(path);
  return new Path(*path);
}

void path_free(Path* path)
{
  ASSERT(path);
  delete path;
}

void path_set_join(Path* path, int join)
{
  path->join = join;
}

void path_set_cap(Path* path, int cap)
{
  path->cap = cap;
}

void path_moveto(Path* path, double x, double y)
{
  int n = path_get_element(path, x, y);
  if (n < 0)
    return;

  if ((n > 0) &&
      ((path->bpath[n-1].code == ART_MOVETO_OPEN) ||
       (path->bpath[n-1].code == ART_MOVETO)))
    n = --path->end;

  path->bpath[n].code = ART_MOVETO_OPEN;
  path->bpath[n].x3 = x;
  path->bpath[n].y3 = y;
}

void path_lineto(Path* path, double x, double y)
{
  int n;

  n = path_get_element(path, x, y);
  if (n < 0)
    return;

  path->bpath[n].code = ART_LINETO;
  path->bpath[n].x3 = x;
  path->bpath[n].y3 = y;
}

void path_curveto(Path* path,
		  double control_x1, double control_y1,
		  double control_x2, double control_y2,
		  double end_x, double end_y)
{
  int n;

  n = path_get_element (path, end_x, end_y);
  if (n < 0)
    return;

  path->bpath[n].code = ART_CURVETO;
  path->bpath[n].x1 = control_x1;
  path->bpath[n].y1 = control_y1;
  path->bpath[n].x2 = control_x2;
  path->bpath[n].y2 = control_y2;
  path->bpath[n].x3 = end_x;
  path->bpath[n].y3 = end_y;
}

void path_close(Path* path)
{
  int n;

  for (n=path->end; n>=0; n--) {
    if (path->bpath[n].code == ART_MOVETO_OPEN) {
      path->bpath[n].code = ART_MOVETO;

      if (path->bpath[path->end-1].x3 != path->bpath[n].x3 ||
	  path->bpath[path->end-1].y3 != path->bpath[n].y3)
	path_lineto (path, path->bpath[n].x3, path->bpath[n].y3);

      break;
    }
  }
}

void path_move(Path* path, double x, double y)
{
  int n;

  for (n=0; n<path->end; n++) {
    path->bpath[n].x1 += x;
    path->bpath[n].y1 += y;
    path->bpath[n].x2 += x;
    path->bpath[n].y2 += y;
    path->bpath[n].x3 += x;
    path->bpath[n].y3 += y;
  }
}

void path_stroke(Path* path, Image *image, int color, double brush_size)
{
  draw_path (path, image, color, brush_size, false);
}

void path_fill(Path* path, Image *image, int color)
{
  draw_path (path, image, color, 0, true);
}

static int path_get_element(Path* path, double x, double y)
{
  int n = path->end;

  if (n+1 >= path->size) {
    path->size += 16;
    path->bpath = art_renew (path->bpath, ArtBpath, path->size);
    if (!path->bpath)
      return -1;
  }

  path->bpath[++path->end].code = ART_END;

  return n;
}

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

/* Render a sorted vector path into a graymap.
 * Modified by David A. Capello to use with ASE bitmaps.
 */

typedef struct _ArtBitmapSVPData ArtBitmapSVPData;

struct _ArtBitmapSVPData
{
  Image *image;
  ase_uint8 *address;
  int shift;
  int x0, x1;
  int color;
  void (*hline) (Image *image, int x1, int y, int x2, int c1, int c2);
};

static void blend_rgb_hline(Image *image, int x1, int y, int x2, int rgb, int a)
{
  ase_uint32 *address = ((ase_uint32 **)image->line)[y]+x1;
  int x;

  for (x=x1; x<=x2; x++) {
    *address = _rgba_blend_normal (*address, rgb, a);
    address++;
  }
}

static void blend_grayscale_hline(Image *image, int x1, int y, int x2, int k, int a)
{
  ase_uint16 *address = ((ase_uint16 **)image->line)[y]+x1;
  int x;

  for (x=x1; x<=x2; x++) {
    *address = _graya_blend_normal(*address, k, a);
    address++;
  }
}

static void blend_indexed_hline(Image *image, int x1, int y, int x2, int i, int a)
{
  ase_uint8 *address = ((ase_uint8 **)image->line)[y]+x1;
  int x;

  if (a > 128)
    for (x=x1; x<=x2; x++)
      *(address++) = i;
}

static void art_image_svp_callback(void *callback_data, int y, int start,
				   ArtSVPRenderAAStep *steps, int n_steps)
{
  ArtBitmapSVPData *data = (ArtBitmapSVPData *)callback_data;
  int run_x0, run_x1;
  int running_sum = start;
  int x0, x1;
  int k, g;
  int color = data->color;

  x0 = data->x0;
  x1 = data->x1;

  if (n_steps > 0) {
    run_x1 = steps[0].x;
    if (run_x1 > x0) {
      g = running_sum>>16;
      if (g > 0)
	(*data->hline) (data->image, x0, y, run_x1-1, color, g);
    }

    for (k = 0; k < n_steps - 1; k++) {
      running_sum += steps[k].delta;
      run_x0 = run_x1;
      run_x1 = steps[k + 1].x;
      if (run_x1 > run_x0) {
	g = running_sum>>16;
	if (g > 0)
	  (*data->hline) (data->image, run_x0, y, run_x1-1, color, g);
      }
    }
    running_sum += steps[k].delta;
    if (x1 > run_x1) {
      g = running_sum>>16;
      if (g > 0)
	(*data->hline) (data->image, run_x1, y, x1-1, color, g);
    }
  }
  else {
    g = running_sum>>16;
    if (g > 0)
      (*data->hline) (data->image, x0, y, x1-1, color, g);
  }

  data->address += data->image->w << data->shift;
}

/**
 * art_gray_svp_aa: Render the vector path into the bytemap.
 * @svp: The SVP to render.
 * @x0: The view window's left coord.
 * @y0: The view window's top coord.
 * @x1: The view window's right coord.
 * @y1: The view window's bottom coord.
 * @buf: The buffer where the bytemap is stored.
 * @rowstride: the rowstride for @buf.
 *
 * Each pixel gets a value proportional to the area within the pixel
 * overlapping the (filled) SVP. Pixel (x, y) is stored at:
 *
 *    @buf[(y - * @y0) * @rowstride + (x - @x0)]
 *
 * All pixels @x0 <= x < @x1, @y0 <= y < @y1 are generated. A
 * stored value of zero is no coverage, and a value of 255 is full
 * coverage. The area within the pixel (x, y) is the region covered
 * by [x..x+1] and [y..y+1].
 **/
static void art_image_svp_aa (const ArtSVP *svp,
			      int x0, int y0, int x1, int y1,
			      Image *image, int color)
{
  ArtBitmapSVPData data;

  switch (image->imgtype) {

    case IMAGE_RGB:
      data.shift = 2;
      data.hline = blend_rgb_hline;
      break;

    case IMAGE_GRAYSCALE:
      data.shift = 1;
      data.hline = blend_grayscale_hline;
      break;

    case IMAGE_INDEXED:
      data.shift = 0;
      data.hline = blend_indexed_hline;
      break;

    /* TODO make something for IMAGE_BITMAP */
    default:
      return;
  }

  data.image = image;
  data.address = ((ase_uint8 *)image->line[y0]) + (x0<<data.shift);
  data.x0 = x0;
  data.x1 = x1+1;
  data.color = color;

  art_svp_render_aa (svp, x0, y0, x1+1, y1+1, art_image_svp_callback, &data);
}

static void draw_path (Path* path, Image *image, int color, double brush_size, int fill)
{
  int w = image->w;
  int h = image->h;
  ArtBpath *bpath;
  ArtVpath *vpath, *old_vpath;
  ArtSVP *svp, *old_svp;
  double matrix[6];

  art_affine_identity (matrix);

  bpath = art_bpath_affine_transform (path->bpath, matrix);
  vpath = art_bez_path_to_vec (bpath, 0.25);
  art_free (bpath);

  vpath = art_vpath_perturb (old_vpath=vpath);
  art_free (old_vpath);

  if (fill) {
    svp = art_svp_from_vpath (vpath);
    art_free (vpath);

    svp = art_svp_uncross (old_svp=svp);
    art_free (old_svp);

    svp = art_svp_rewind_uncrossed (old_svp=svp, ART_WIND_RULE_ODDEVEN);
    art_free (old_svp);
  }
  else {
/*     vpath = art_vpath_perturb (old_vpath=vpath); */
/*     art_free (old_vpath); */

    svp = art_svp_vpath_stroke
      (vpath,
       path->join == PATH_JOIN_MITER ? ART_PATH_STROKE_JOIN_MITER:
       path->join == PATH_JOIN_ROUND ? ART_PATH_STROKE_JOIN_ROUND:
       path->join == PATH_JOIN_BEVEL ? ART_PATH_STROKE_JOIN_BEVEL: (ArtPathStrokeJoinType)0,
       path->cap == PATH_CAP_BUTT ? ART_PATH_STROKE_CAP_BUTT:
       path->cap == PATH_CAP_ROUND ? ART_PATH_STROKE_CAP_ROUND:
       path->cap == PATH_CAP_SQUARE ? ART_PATH_STROKE_CAP_SQUARE: (ArtPathStrokeCapType)0,
       brush_size, 1.0, 0.25);
/*        brush_size, 4.0, 0.25); */
/*        brush_size, 11.0, 0.25); */

    art_free (vpath);
  }

  art_image_svp_aa (svp, 0, 0, w-1, h-1, image, color);
  art_free (svp);
}
