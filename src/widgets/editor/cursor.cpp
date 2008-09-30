/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#include <assert.h>
#include <allegro.h>

#include "jinete/jbase.h"
#include "jinete/jlist.h"
#include "jinete/jrect.h"
#include "jinete/jregion.h"
#include "jinete/jsystem.h"
#include "jinete/jwidget.h"

#include "core/color.h"
#include "modules/tools.h"
#include "raster/brush.h"
#include "util/boundary.h"
#include "widgets/editor.h"

/**********************************************************************/
/* drawing-cursor routines */
/**********************************************************************/

/**
 * Returns true if the cursor of the editor needs subpixel movement.
 */
#define IS_SUBPIXEL(editor)	((editor)->zoom >= 2)

/**
 * Maximum quantity of colors to save pixels overlapped by the cursor.
 */
#define MAX_SAVED   4096	/* TODO maybe more size is required for
				   high resolutions */

static struct {
  int brush_type;
  int brush_size;
  int brush_angle;
  int nseg;
  BoundSeg *seg;
} cursor_bound = { 0, 0, 0, 0, NULL };

enum { CURSOR_CROSS_ONE   = 1,
       CURSOR_BRUSH	  = 2 };

static int cursor_type;
static int cursor_negative;

static int saved_pixel[MAX_SAVED];
static int saved_pixel_n;
static JRegion clipping_region;
static JRegion old_clipping_region;

static void generate_cursor_boundaries(void);
static void for_each_pixel_of_brush(Editor *editor, int x, int y, int color, void (*pixel)(BITMAP *bmp, int x, int y, int color));

static void editor_cursor_cross(Editor *editor, int x, int y, int color, int thickness, void (*pixel)(BITMAP *bmp, int x, int y, int color));
static void editor_cursor_brush(Editor *editor, int x, int y, int color, void (*pixel)(BITMAP *bmp, int x, int y, int color));

static void savepixel(BITMAP *bmp, int x, int y, int color);
static void drawpixel(BITMAP *bmp, int x, int y, int color);
static void cleanpixel(BITMAP *bmp, int x, int y, int color);

static int point_inside_region(int x, int y, JRegion region);

void editor_cursor_exit(void)
{
  if (cursor_bound.seg != NULL)
    jfree(cursor_bound.seg);
}

/**
 * Draws the brush cursor inside the specified editor.
 *
 * @warning You should clean the cursor before to use
 * this routine with other editor.
 *
 * @param widget The editor widget
 * @param x Absolute position in X axis of the mouse.
 * @param y Absolute position in Y axis of the mouse.
 *
 * @see editor_clean_cursor
 */
void editor_draw_cursor(JWidget widget, int x, int y)
{
  Editor *editor = editor_data(widget);
  int color;

  assert(editor->cursor_thick == 0);

  /* get drawable region */
  clipping_region = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS);

  /* get cursor color */
  cursor_negative = is_cursor_mask();
  color = get_raw_cursor_color();

  /* cursor in the screen (view) */
  editor->cursor_screen_x = x;
  editor->cursor_screen_y = y;
  
  /* get cursor position in the editor */
  screen_to_editor(widget, x, y, &x, &y);

  /* get cursor type */
  if (get_thickness_for_cursor() == 1) {
    cursor_type = CURSOR_CROSS_ONE;
  }
  else {
    switch (get_brush_type()) {
      case BRUSH_CIRCLE:
      case BRUSH_SQUARE:
	cursor_type = CURSOR_BRUSH;
	if ((get_brush_size()<<editor->zoom)/2 > 3+(1<<editor->zoom))
	  cursor_type |= CURSOR_CROSS_ONE;
	break;
      case BRUSH_LINE:
	cursor_type = CURSOR_BRUSH;
	break;
    }
  }

  if (cursor_type & CURSOR_BRUSH)
    generate_cursor_boundaries();

  /* save area and draw the cursor */
  acquire_bitmap(ji_screen);
  ji_screen->clip = FALSE;
  for_each_pixel_of_brush(editor, x, y, color, savepixel);
  for_each_pixel_of_brush(editor, x, y, color, drawpixel);
  ji_screen->clip = TRUE;
  release_bitmap(ji_screen);

  /* cursor thickness */
  editor->cursor_thick = get_thickness_for_cursor();

  /* cursor in the editor (model) */
  editor->cursor_editor_x = x;
  editor->cursor_editor_y = y;

  /* save the clipping-region to know where to clean the pixels */
  old_clipping_region = clipping_region;
}

/**
 * Cleans the brush cursor from the specified editor.
 *
 * The mouse position is got from the last
 * call to @c editor_draw_cursor. So you must
 * to use this routine only if you called
 * @c editor_draw_cursor before with the specified
 * editor @a widget.
 *
 * @param widget The editor widget
 *
 * @see editor_draw_cursor
 */
void editor_clean_cursor(JWidget widget)
{
  Editor *editor = editor_data(widget);
  int x, y;

  assert(editor->cursor_thick != 0);

  clipping_region = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS);

  x = editor->cursor_editor_x;
  y = editor->cursor_editor_y;

  /* restore points */
  acquire_bitmap(ji_screen);
  ji_screen->clip = FALSE;
  for_each_pixel_of_brush(editor, x, y, 0, cleanpixel);
  ji_screen->clip = TRUE;
  release_bitmap(ji_screen);

  editor->cursor_thick = 0;

  jregion_free(clipping_region);
  jregion_free(old_clipping_region);
  clipping_region = NULL;
  old_clipping_region = NULL;
}

/**
 * Returns true if the cursor to draw in the editor has subpixel
 * movement (a little pixel of the screen that indicates where is the
 * mouse inside the pixel of the sprite).
 */
bool editor_cursor_is_subpixel(JWidget widget)
{
  Editor *editor = editor_data(widget);
  return IS_SUBPIXEL(editor);
}

/**********************************************************************/

static void generate_cursor_boundaries(void)
{
  if (cursor_bound.seg == NULL || 
      cursor_bound.brush_type != get_brush_type() ||
      cursor_bound.brush_size != get_brush_size() ||
      cursor_bound.brush_angle != get_brush_angle()) {
    cursor_bound.brush_type = get_brush_type();
    cursor_bound.brush_size = get_brush_size();
    cursor_bound.brush_angle = get_brush_angle();

    if (cursor_bound.seg != NULL)
      jfree(cursor_bound.seg);

    cursor_bound.seg = find_mask_boundary(get_brush()->image,
					  &cursor_bound.nseg,
					  IgnoreBounds, 0, 0, 0, 0);
  }
}

static void for_each_pixel_of_brush(Editor *editor, int x, int y, int color,
				    void (*pixel) (BITMAP *bmp, int x, int y, int color))
{
  saved_pixel_n = 0;

  if (cursor_type & CURSOR_CROSS_ONE) {
    editor_cursor_cross(editor, x, y, color, 1, pixel);
  }

  if (cursor_type & CURSOR_BRUSH) {
    editor_cursor_brush(editor, x, y, color, pixel);
  }

  if (IS_SUBPIXEL(editor)) {
    (*pixel)(ji_screen,
	     editor->cursor_screen_x,
	     editor->cursor_screen_y, color);
  }
}

/**********************************************************************/
/* cross */

static void editor_cursor_cross(Editor *editor, int x, int y, int color, int thickness, void (*pixel) (BITMAP *bmp, int x, int y, int color))
{
  static int cursor_cross[6*6] = {
    0, 0, 1, 1, 0, 0,
    0, 0, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    0, 0, 1, 1, 0, 0,
    0, 0, 1, 1, 0, 0,
  };
  int u, v, xout, yout;

  for (v=0; v<6; v++) {
    for (u=0; u<6; u++) {
      if (cursor_cross[v*6+u]) {
	editor_to_screen(editor->widget, x, y, &xout, &yout);

	xout += ((u<3) ?
		 u-((thickness>>1)<<editor->zoom)-3:
		 u-((thickness>>1)<<editor->zoom)-3+(thickness<<editor->zoom));

	yout += ((v<3)?
		 v-((thickness>>1)<<editor->zoom)-3:
		 v-((thickness>>1)<<editor->zoom)-3+(thickness<<editor->zoom));

	(*pixel)(ji_screen, xout, yout, color);
      }
    }
  }
}

/**********************************************************************/
/* brush */

static void editor_cursor_brush(Editor *editor, int x, int y, int color, void (*pixel) (BITMAP *bmp, int x, int y, int color))
{
  int c, x1, y1, x2, y2;
  BoundSeg *seg;

  for (c=0; c<cursor_bound.nseg; c++) {
    seg = cursor_bound.seg+c;

    x1 = seg->x1 - cursor_bound.brush_size/2;
    y1 = seg->y1 - cursor_bound.brush_size/2;
    x2 = seg->x2 - cursor_bound.brush_size/2;
    y2 = seg->y2 - cursor_bound.brush_size/2;

    editor_to_screen(editor->widget, x+x1, y+y1, &x1, &y1);
    editor_to_screen(editor->widget, x+x2, y+y2, &x2, &y2);

    if (seg->open) {		/* outside */
      if (x1 == x2) {
	x1--;
	x2--;
	y2--;
      }
      else {
	y1--;
	y2--;
	x2--;
      }
    }
    else {
      if (x1 == x2) {
	y2--;
      }
      else {
	x2--;
      }
    }

    do_line(ji_screen, x1, y1, x2, y2, color, pixel);
  }
}

/**********************************************************************/
/* extra routines */

static void savepixel(BITMAP *bmp, int x, int y, int color)
{
  if (saved_pixel_n < MAX_SAVED && point_inside_region(x, y, clipping_region))
    saved_pixel[saved_pixel_n++] = getpixel(bmp, x, y);
}

static void drawpixel(BITMAP *bmp, int x, int y, int color)
{
  if (saved_pixel_n < MAX_SAVED && point_inside_region(x, y, clipping_region)) {
    if (cursor_negative) {
      int r, g, b, c = saved_pixel[saved_pixel_n++];

      r = getr(c);
      g = getg(c);
      b = getb(c);

      putpixel(bmp, x, y, blackandwhite_neg(r, g, b));
    }
    else {
      putpixel(bmp, x, y, color);
    }
  }
}

static void cleanpixel(BITMAP *bmp, int x, int y, int color)
{
  if (saved_pixel_n < MAX_SAVED) {
    if (point_inside_region(x, y, clipping_region))
      putpixel(bmp, x, y, saved_pixel[saved_pixel_n++]);
    else if (point_inside_region(x, y, old_clipping_region))
      saved_pixel_n++;
  }
}

static int point_inside_region(int x, int y, JRegion region)
{
  struct jrect box;
  return jregion_point_in(region, x, y, &box);
}
