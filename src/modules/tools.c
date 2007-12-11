/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>
#include <limits.h>
#include <math.h>

#include "jinete/jalert.h"
#include "jinete/jlist.h"
#include "jinete/jmanager.h"
#include "jinete/jregion.h"
#include "jinete/jsystem.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"

#include "core/app.h"
#include "core/cfg.h"
#include "modules/color.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "modules/sprites.h"
#include "modules/tools.h"
#include "raster/algo.h"
#include "raster/blend.h"
#include "raster/brush.h"
#include "raster/dirty.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"
#include "raster/undo.h"
#include "util/misc.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"
#include "widgets/toolbar.h"

#endif

Tool *current_tool = NULL;

static Brush *brush = NULL;
static int brush_mode;
static int glass_dirty;
static int spray_width;
static int air_speed;
static bool filled_mode;
static bool tiled_mode;
static bool use_grid;
static bool view_grid;
static JRect grid;
static bool onionskin;

static char *cursor_color = NULL;
static int _cursor_color;
static int _cursor_mask;

static Image *tool_image = NULL;
static int tool_color;

static void update_cursor_color(void)
{
  if (cursor_color) {
    if (ji_screen)
      _cursor_color = get_color_for_allegro(bitmap_color_depth(ji_screen),
					    cursor_color);
    else
      _cursor_color = 0;

    _cursor_mask = (ustrcmp(cursor_color, "mask") == 0);
  }
}

int init_module_tools(void)
{
  int size, type, angle;

  /* current tool */
  current_tool = &ase_tool_brush;

  /* brush */
  brush = brush_new();

  type = get_config_int("Tools", "BrushType", BRUSH_CIRCLE);
  size = get_config_int("Tools", "BrushSize", 1);
  angle = get_config_int("Tools", "BrushAngle", 0);
  brush_mode = get_config_int("Tools", "BrushMode", DRAWMODE_OPAQUE);

  brush_set_type(brush, MID (BRUSH_CIRCLE, type, BRUSH_LINE));
  brush_set_size(brush, MID (1, size, 32));
  brush_set_angle(brush, MID (0, angle, 360));

  /* cursor color */
  set_cursor_color(get_config_string("Tools", "CursorColor", "mask"));

  /* tools configuration */
  glass_dirty = get_config_int("Tools", "GlassDirty", 128);
  spray_width = get_config_int("Tools", "SprayWidth", 16);
  air_speed   = get_config_int("Tools", "AirSpeed", 75);
  filled_mode = get_config_bool("Tools", "Filled", FALSE);
  tiled_mode  = get_config_bool("Tools", "Tiled", FALSE);
  use_grid    = get_config_bool("Tools", "UseGrid", FALSE);
  view_grid   = get_config_bool("Tools", "ViewGrid", FALSE);
  onionskin   = get_config_bool("Tools", "Onionskin", FALSE);

  grid = jrect_new(0, 0, 16, 16);
  get_config_rect("Tools", "GridRect", grid);
  set_grid(grid);

  glass_dirty = MID(0, glass_dirty, 255);
  spray_width = MID(1, spray_width, 320);
  air_speed   = MID(1, air_speed, 100);

  refresh_tools_names();
  hook_palette_changes(update_cursor_color);

  return 0;
}

void exit_module_tools(void)
{
  set_config_string("Tools", "CursorColor", cursor_color);
  set_config_int("Tools", "GlassDirty", glass_dirty);
  set_config_int("Tools", "SprayWidth", spray_width);
  set_config_int("Tools", "AirSpeed", air_speed);
  set_config_bool("Tools", "Filled", filled_mode);
  set_config_bool("Tools", "Tiled", tiled_mode);
  set_config_bool("Tools", "UseGrid", use_grid);
  set_config_bool("Tools", "ViewGrid", view_grid);
  set_config_rect("Tools", "GridRect", grid);
  set_config_bool("Tools", "Onionskin", onionskin);

  set_config_int("Tools", "BrushType", brush->type);
  set_config_int("Tools", "BrushSize", brush->size);
  set_config_int("Tools", "BrushAngle", brush->angle);
  set_config_int("Tools", "BrushMode", brush_mode);

  jrect_free(grid);
  brush_free(brush);
  jfree(cursor_color);

  unhook_palette_changes(update_cursor_color);
}

void refresh_tools_names(void)
{
  int c;

  for (c=0; ase_tools_list[c]; c++)
    ase_tools_list[c]->translated_name = _(ase_tools_list[c]->name);
}

void select_tool(Tool *tool)
{
  ASSERT(tool != NULL);
  
  current_tool = tool;

  /* update status-bar */
  if (app_get_status_bar() &&
      jwidget_is_visible(app_get_status_bar()))
    status_bar_set_text(app_get_status_bar(), 500, "%s: %s",
			_("Tool"), current_tool->translated_name);

  /* update tool-bar */
  if (app_get_tool_bar())
    tool_bar_update(app_get_tool_bar());
}

void select_tool_by_name(const char *tool_name)
{
  int c;

  for (c=0; ase_tools_list[c]; c++) {
    if (ustricmp(ase_tools_list[c]->name, tool_name) == 0) {
      if (current_tool == ase_tools_list[c])
	return;

      select_tool(ase_tools_list[c]);
      break;
    }
  }
}

Brush *get_brush(void) { return brush; }
int get_brush_type(void) { return brush->type; }
int get_brush_size(void) { return brush->size; }
int get_brush_angle(void) { return brush->angle; }
int get_brush_mode(void) { return brush_mode; }
int get_glass_dirty(void) { return glass_dirty; }
int get_spray_width(void) { return spray_width; }
int get_air_speed(void) { return air_speed; }
bool get_filled_mode(void) { return filled_mode; }
bool get_tiled_mode(void) { return tiled_mode; }
bool get_use_grid(void) { return use_grid; }
bool get_view_grid(void) { return view_grid; }

JRect get_grid(void)
{
  return jrect_new_copy (grid);
}

bool get_onionskin(void)
{
  return onionskin;
}

void set_brush_type(int type) { brush_set_type (brush, type); }
void set_brush_size(int size) { brush_set_size (brush, size); }
void set_brush_angle(int angle) { brush_set_angle (brush, angle); }
void set_brush_mode(int mode) { brush_mode = mode; }
void set_glass_dirty(int new_glass_dirty) { glass_dirty = new_glass_dirty; }
void set_spray_width(int new_spray_width) { spray_width = new_spray_width; }
void set_air_speed(int new_air_speed) { air_speed = new_air_speed; }
void set_filled_mode(bool status) { filled_mode = status; }
void set_tiled_mode(bool status) { tiled_mode = status; }
void set_use_grid(bool status) { use_grid = status; }
void set_view_grid(bool status) { view_grid = status; }

void set_grid(JRect rect)
{
  jrect_copy(grid, rect);
  if (grid->x2 <= grid->x1) grid->x2 = grid->x1+1;
  if (grid->y2 <= grid->y1) grid->y2 = grid->y1+1;
}

void set_onionskin(bool status)
{
  onionskin = status;
}

int get_raw_cursor_color(void)
{
  return _cursor_color;
}

bool is_cursor_mask(void)
{
  return _cursor_mask;
}

const char *get_cursor_color(void)
{
  return cursor_color;
}

void set_cursor_color(const char *color)
{
  if (cursor_color)
    jfree(cursor_color);

  cursor_color = jstrdup(color);

  update_cursor_color();
}

/* returns the size which use the current tool */
int get_thickness_for_cursor(void)
{
  /* 1 pixel of thickness */
  if ((current_tool == &ase_tool_marker) ||
      (current_tool == &ase_tool_floodfill))
    return 1;
  /* the spray have a special thickness (for the spray-width) */
/*   else if (current_tool == &ase_tool_spray) */
/*     return brush->size+spray_width*2; */
  /* all the other tools use the original thickness */
  else
    return brush->size;
}

/***********************************************************/
/* MARKER                                                  */
/***********************************************************/

Tool ase_tool_marker =
{
  "Marker",
#if 0
  _("Marker"),
#else
  NULL,
#endif
  TOOL_FIRST2LAST | TOOL_UPDATE_BOX,
  NULL
};

/***********************************************************/
/* DOTS                                                    */
/***********************************************************/

static void tool_dots_put (Dirty *dirty, int x1, int y1, int x2, int y2)
{
  dirty_putpixel_brush (dirty, brush, x2, y2);
}

Tool ase_tool_dots =
{
  "Dots",
#if 0
  _("Dots"),
#else
  NULL,
#endif
  TOOL_ACCUMULATE_DIRTY | TOOL_OLD2LAST | TOOL_UPDATE_POINT,
  tool_dots_put
};

/***********************************************************/
/* PENCIL                                                  */
/***********************************************************/

static void tool_pencil_put(Dirty *dirty, int x1, int y1, int x2, int y2)
{
  dirty_line_brush(dirty, brush, x1, y1, x2, y2);
}

Tool ase_tool_pencil =
{
  "Pencil",
#if 0
  _("Pencil"),
#else
  NULL,
#endif
  TOOL_ACCUMULATE_DIRTY | TOOL_OLD2LAST | TOOL_UPDATE_TRACE,
  tool_pencil_put
};

/***********************************************************/
/* BRUSH                                                   */
/***********************************************************/

Tool ase_tool_brush =
{
  "Brush",
#if 0
  _("Brush"),
#else
  NULL,
#endif
  TOOL_ACCUMULATE_DIRTY | TOOL_FOURCHAIN | TOOL_UPDATE_LAST4,
  NULL
};

/***********************************************************/
/* FLOODFILL                                               */
/***********************************************************/

static void tool_floodfill_hline (int x1, int y, int x2, Dirty *dirty)
{
  dirty_hline(dirty, x1, y, x2);
}

static void tool_floodfill_put(Dirty *dirty, int x1, int y1, int x2, int y2)
{
  if (image_getpixel(tool_image, x2, y2) != tool_color) {
    int tiled = dirty->tiled;
    dirty->tiled = FALSE;

    algo_floodfill(tool_image, x2, y2, dirty,
		   (AlgoHLine)tool_floodfill_hline);

    dirty->tiled = tiled;
  }
}

Tool ase_tool_floodfill =
{
  "Floodfill",
#if 0
  _("Floodfill"),
#else
  NULL,
#endif
  TOOL_ACCUMULATE_DIRTY | TOOL_OLD2LAST | TOOL_UPDATE_ALL,
  tool_floodfill_put
};

/***********************************************************/
/* SPRAY                                                   */
/***********************************************************/

static void tool_spray_put (Dirty *dirty, int x1, int y1, int x2, int y2)
{
  int c, x, y, times = (spray_width*spray_width/4) * air_speed / 100;

#ifdef __MINGW32__             /* MinGW32 has a RAND_MAX too small */
  fixed angle, radius;

  for (c=0; c<times; c++) {
    angle = itofix (rand () * 256 / RAND_MAX);
    radius = itofix (rand () * (spray_width*10) / RAND_MAX) / 10;
    x = fixtoi (fixmul (radius, fixcos (angle)));
    y = fixtoi (fixmul (radius, fixsin (angle)));
    dirty_putpixel_brush (dirty, brush, x2+x, y2+y);
  }
#else
  fixed angle, radius;

  for (c=0; c<times; c++) {
    angle = rand ();
    radius = rand () % itofix (spray_width);
    x = fixtoi (fixmul (radius, fixcos (angle)));
    y = fixtoi (fixmul (radius, fixsin (angle)));
    dirty_putpixel_brush (dirty, brush, x2+x, y2+y);
  }
#endif
}

Tool ase_tool_spray =
{
  "Spray",
#if 0
  _("Spray"),
#else
  NULL,
#endif
  TOOL_ACCUMULATE_DIRTY | TOOL_OLD2LAST | TOOL_UPDATE_SPRAY,
  tool_spray_put
};

/***********************************************************/
/* LINE                                                    */
/***********************************************************/

static void tool_line_put (Dirty *dirty, int x1, int y1, int x2, int y2)
{
  dirty_line_brush (dirty, brush, x1, y1, x2, y2);
}

Tool ase_tool_line =
{
  "Line",
#if 0
  _("Line"),
#else
  NULL,
#endif
  TOOL_FIRST2LAST | TOOL_UPDATE_BOX | TOOL_EIGHT_ANGLES,
  tool_line_put
};

/***********************************************************/
/* RECTANGLE                                               */
/***********************************************************/

static void tool_rectangle_put (Dirty *dirty, int x1, int y1, int x2, int y2)
{
  int c;

  if (x1 > x2) {
    c = x1;
    x1 = x2;
    x2 = c;
  }

  if (y1 > y2) {
    c = y1;
    y1 = y2;
    y2 = c;
  }

#if 1
  dirty_hline_brush (dirty, brush, x1, y1, x2);
  dirty_hline_brush (dirty, brush, x1, y2, x2);
  for (c=y1; c<y2; c++) {
    dirty_putpixel_brush (dirty, brush, x1, c);
    dirty_putpixel_brush (dirty, brush, x2, c);
  }

  if (filled_mode)
    dirty_rectfill (dirty, x1, y1, x2, y2);
#else  /* TODO rpoly */
  if (x2 != x1 && y2 != y1) {
    fixed angle, new_x, new_y, old_x, old_y, fst_x, fst_y;
    fixed start = itofix (-64);
    fixed step = fixdiv (itofix (256), itofix (5));
    fixed end = fixadd (itofix (256), start);
    int fst = TRUE;

    for (angle=start; angle<end; angle=fixadd (angle, step)) {
      new_x = fixadd (itofix ((x1+x2)/2), fixmul (fixcos (angle), itofix ((x2-x1-1)/2)));
      new_y = fixadd (itofix ((y1+y2)/2), fixmul (fixsin (angle), itofix ((y2-y1-1)/2)));

      if (fst) {
	fst = FALSE;
	fst_x = new_x;
	fst_y = new_y;
      }
      else {
	dirty_line_brush (dirty, brush,
			  fixtoi (old_x), fixtoi (old_y),
			  fixtoi (new_x), fixtoi (new_y));
      }

      old_x = new_x;
      old_y = new_y;
    }

    dirty_line_brush (dirty, brush,
		      fixtoi (old_x), fixtoi (old_y),
		      fixtoi (fst_x), fixtoi (fst_y));
  }
  else {
    dirty_line_brush (dirty, brush, x1, y1, x2, y2);
  }
#endif
}

Tool ase_tool_rectangle =
{
  "Rectangle",
#if 0
  _("Rectangle"),
#else
  NULL,
#endif
  TOOL_FIRST2LAST | TOOL_UPDATE_BOX | TOOL_ACCEPT_FILL,
  tool_rectangle_put
};

/***********************************************************/
/* ELLIPSE                                                 */
/***********************************************************/

static void tool_ellipse_pixel (int x, int y, Dirty *dirty)
{
  dirty_putpixel_brush (dirty, brush, x, y);
}

static void tool_ellipse_hline (int x1, int y, int x2, Dirty *dirty)
{
#if 0
  dirty_hline_brush (dirty, brush, x1, y, x2);
#else
  dirty_hline (dirty, x1, y, x2);
#endif
}

static void tool_ellipse_put (Dirty *dirty, int x1, int y1, int x2, int y2)
{
#if 0
  if (!filled_mode)
    algo_ellipse (x1, y1, x2, y2, dirty, (AlgoPixel)tool_ellipse_pixel);
  else
    algo_ellipsefill (x1, y1, x2, y2, dirty, (AlgoHLine)tool_ellipse_hline);
#else
  algo_ellipse (x1, y1, x2, y2, dirty, (AlgoPixel)tool_ellipse_pixel);

  if (filled_mode)
    algo_ellipsefill (x1, y1, x2, y2, dirty, (AlgoHLine)tool_ellipse_hline);
#endif
}

Tool ase_tool_ellipse =
{
  "Ellipse",
#if 0
  _("Ellipse"),
#else
  NULL,
#endif
  TOOL_FIRST2LAST | TOOL_UPDATE_BOX | TOOL_ACCEPT_FILL,
  tool_ellipse_put
};

/***********************************************************/
/* TOOL'S LIST                                             */
/***********************************************************/

Tool *ase_tools_list[] =
{
  &ase_tool_marker,
  &ase_tool_dots,
  &ase_tool_pencil,
  &ase_tool_brush,
  &ase_tool_floodfill,
  &ase_tool_spray,
  &ase_tool_line,
  &ase_tool_rectangle,
  &ase_tool_ellipse,
  NULL
};

/***********************************************************/
/* TOOL CONTROL                                            */
/***********************************************************/

/* static void apply_grid (int *x, int *y); */
static void fourchain_line(int x1, int y1, int x2, int y2, void *data);

static void my_image_hline4_opaque(int x1, int y, int x2, void *data);
static void my_image_hline2_opaque(int x1, int y, int x2, void *data);
static void my_image_hline1_opaque(int x1, int y, int x2, void *data);

static void my_image_hline4_glass(int x1, int y, int x2, void *data);
static void my_image_hline2_glass(int x1, int y, int x2, void *data);
static void my_image_hline1_glass(int x1, int y, int x2, void *data);

static void my_image_hline4_semi(int x1, int y, int x2, void *data);
static void my_image_hline2_semi(int x1, int y, int x2, void *data);
static void my_image_hline1_semi(int x1, int y, int x2, void *data);

static AlgoHLine drawmode_procs[][3] =
{
  { my_image_hline4_opaque, my_image_hline4_glass, my_image_hline4_semi },
  { my_image_hline2_opaque, my_image_hline2_glass, my_image_hline2_semi },
  { my_image_hline1_opaque, my_image_hline1_glass, my_image_hline1_semi }
};

static void *rect_data = NULL;

static void marker_scroll_callback(int before_change)
{
  if (before_change && rect_data) {
    rectrestore(rect_data);
    rectdiscard(rect_data);
    rect_data = NULL;
  }
}

/* controls any tool to draw in the current sprite */
void control_tool(JWidget widget, Tool *tool, const char *_color)
{
  Editor *editor = editor_data(widget);
  JWidget status_bar = app_get_status_bar();
  Dirty *dirty = NULL;
  int x1, y1, x2, y2;
  int old_x1, old_y1, old_x2, old_y2;
  int outx1, outy1, outx2, outy2;
  int mouse_x[4];
  int mouse_y[4];
  int c, pts[8];
  Image *image;
  int start_x, new_x, offset_x;
  int start_y, new_y, offset_y;
  int start_b;
  int color;
  int old_key_shifts, update, forced_update, first_time;
  int spray_time;
  AlgoHLine hline_proc;
  const char *_pos = _("Pos");
  const char *_start = _("Start");
  const char *_end = _("End");
  const char *_size = _("Size");
  bool click2 = get_config_bool("Options", "DrawClick2", FALSE);

  /* get image information */
  image = GetImage2(editor->sprite, &offset_x, &offset_y, NULL);

  /* we have a image layer to paint in? */
  if (!image)
    return;
  /* error, the active layer is not visible */
  else if (!editor->sprite->layer->readable) {
    jalert(_("Warning"
	     "<<The active layer is hidden,"
	     "<<make it visible and try again"
	     "||&Close"));
    return;
  }
  /* error, the active layer is locked */
  else if (!editor->sprite->layer->writable) {
    jalert(_("Warning"
	     "<<The active layer is locked,"
	     "<<unlock it and try again"
	     "||&Close"));
    return;
  }

  /* select the hline procedure */
  hline_proc = drawmode_procs[MID (0, image->imgtype, 2)]
			     [MID (0, brush_mode, 2)];

  /* alignment offset */
  offset_x = -offset_x;
  offset_y = -offset_y;

  /* get the color to use for the image */
  color = get_color_for_image (image->imgtype, _color);

  /* global stuff needs */
  tool_image = image;
  tool_color = color;

  /* accumulative dirty */
  if (tool->flags & TOOL_ACCUMULATE_DIRTY) {
    dirty = dirty_new (image, 0, 0, image->w-1, image->h-1, tiled_mode);
    dirty->mask = (editor->sprite->mask &&
		   editor->sprite->mask->bitmap)? editor->sprite->mask: NULL;
  }

  update = FALSE;
  forced_update = TRUE;
  first_time = TRUE;
  old_key_shifts = key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG);
  spray_time = ji_clock;
  old_x1 = old_y1 = old_x2 = old_y2 = 0;

  /* start click */
  editor_click_start(widget,
		     click2 ? MODE_CLICKANDCLICK:
			      MODE_CLICKANDRELEASE,
		     &start_x, &start_y, &start_b);

  for (c=0; c<4; c++) {
    mouse_x[c] = start_x;
    mouse_y[c] = start_y;
  }

  do {
    if ((update) || (forced_update)) {
      int real_update = (current_tool != &ase_tool_spray) || (forced_update);

      if (!first_time) {
	for (c=3; c>0; c--) {
	  mouse_x[c] = mouse_x[c-1];
	  mouse_y[c] = mouse_y[c-1];
	}
	mouse_x[0] = new_x;
	mouse_y[0] = new_y;
      }

      /* common behavior for boxes likes tools */
      if (tool->flags & TOOL_FIRST2LAST) {
	x1 = start_x;
	y1 = start_y;
	x2 = mouse_x[0];
	y2 = mouse_y[0];

	/* grid */
        if (use_grid) {
	  apply_grid(&x1, &y1);
	  apply_grid(&x2, &y2);
        }

	/* square aspect */
	if (old_key_shifts & KB_SHIFT_FLAG) {
	  int dx = x2 - x1;
	  int dy = y2 - y1;
	  int size;

	  if (tool->flags & TOOL_EIGHT_ANGLES)
	    size = MAX (ABS (dx), ABS (dy));
	  else
	    size = MIN (ABS (dx), ABS (dy));

	  x2 = x1 + SGN (dx) * size;
	  y2 = y1 + SGN (dy) * size;

	  if (tool->flags & TOOL_EIGHT_ANGLES) {
	    if (ABS (dx) <= ABS (dy)/2)
	      x2 = x1;
	    else if (ABS (dy) <= ABS (dx)/2)
	      y2 = y1;
	  }
	}

	/* center */
	if (old_key_shifts & KB_CTRL_FLAG) {
	  int cx = x1;
	  int cy = y1;
	  int rx = x2 - x1;
	  int ry = y2 - y1;
	  x1 = cx - rx;
	  y1 = cy - ry;
	  x2 = cx + rx;
	  y2 = cy + ry;
	}

	x1 += offset_x;
	y1 += offset_y;
	x2 += offset_x;
	y2 += offset_y;
      }
      /* common behavior for pencil like tools */
      else if (tool->flags & TOOL_OLD2LAST) {
	x1 = mouse_x[1];
	y1 = mouse_y[1];
	x2 = mouse_x[0];
	y2 = mouse_y[0];

	/* grid */
        if (use_grid && (tool != &ase_tool_floodfill)) {
	  apply_grid(&x1, &y1);
	  apply_grid(&x2, &y2);
        }

	x1 += offset_x;
	y1 += offset_y;
	x2 += offset_x;
	y2 += offset_y;
      }
      /* special behavior for brush */
      else if (tool->flags & TOOL_FOURCHAIN) {
        for (c=0; c<4; c++) {
	  pts[c*2  ] = mouse_x[c];
	  pts[c*2+1] = mouse_y[c];

	  /* grid */
	  if (use_grid)
	    apply_grid(&pts[c*2], &pts[c*2+1]);

	  pts[c*2  ] += offset_x;
	  pts[c*2+1] += offset_y;
        }

        x1 = pts[0];
        y1 = pts[1];
      }
      else {
        x1 = y1 = x2 = y2 = 0;
      }

      if (real_update) {
	/* no accumulative dirty */
	if (!(tool->flags & TOOL_ACCUMULATE_DIRTY)) {
	  if (dirty) {
	    dirty_put (dirty);
	    dirty_free (dirty);
	  }

	  dirty = dirty_new(image, 0, 0, image->w-1, image->h-1, tiled_mode);
	  dirty->mask = (editor->sprite->mask &&
			 editor->sprite->mask->bitmap)? editor->sprite->mask: NULL;
	}

	/* displace region */
	if (dirty->mask)
	  mask_move(dirty->mask, offset_x, offset_y);

	/* create the area which the trace will dirty */
	if (tool == &ase_tool_brush) {
	  algo_spline(pts[0], pts[1], pts[2], pts[3],
		      pts[4], pts[5], pts[6], pts[7],
		      dirty, fourchain_line);
	}
	else if (tool->put) {
	  tool->put(dirty, x1, y1, x2, y2);
	}

	/* displace region */
	if (dirty->mask)
	  mask_move(dirty->mask, -offset_x, -offset_y);

	/* get the background which the trace will overlap */
	dirty_get(dirty);

	/* draw the trace */
	algo_dirty(dirty, NULL, hline_proc);
      }

      /* prepare */
      acquire_bitmap(ji_screen);

      /* clean the area occupied by the cursor in the screen */
      if (editor->cursor_thick)
	editor_clean_cursor(widget);

/*       /\* for Path *\/ */
/*       if (tool == &ase_tool_path) { */
/* 	if (first_time) { */
/* 	  node->mode = PATH_NODE_ANGLED; */
/* 	  node->px = node->x = node->nx = x1; */
/* 	  node->py = node->y = node->ny = y1; */

/* 	  if (!editor->sprite->path) { */
/* 	    Path *path = path_new ("*working*"); */
/* 	    sprite_set_path (editor->sprite, path); */
/* 	  } */
/* 	  else { */
/* 	    PathNode *last = jlist_last (editor->sprite->path->nodes)->data; */
/* 	    last->n = node; */
/* 	    node->p = last; */
/* 	  } */

/* 	  path_append_node (editor->sprite->path, node); */
/* 	} */
/* 	else { */
/* 	  node->mode = PATH_NODE_SMOOTH_CURVE; */
/* 	  node->nx = x2; */
/* 	  node->ny = y2; */
/* 	  node->px = node->x - (node->nx - node->x); */
/* 	  node->py = node->y - (node->ny - node->y); */
/* 	} */
/*       } */
      if (real_update) {
	/* for Marker */
	if (tool == &ase_tool_marker) {
	  /* draw the rectangle mark */
	  JRegion region;
	  int nrects;
	  JRect rc;

	  editor_to_screen (widget,
			    MIN (x1, x2)-offset_x,
			    MIN (y1, y2)-offset_y, &outx1, &outy1);

	  editor_to_screen (widget,
			    MAX (x1, x2)-offset_x,
			    MAX (y1, y2)-offset_y, &outx2, &outy2);

	  outx2 += (1<<editor->zoom)-1;
	  outy2 += (1<<editor->zoom)-1;

	  if (rect_data) {
	    rectrestore (rect_data);
	    rectdiscard (rect_data);
	  }
	  rect_data = rectsave (ji_screen, outx1, outy1, outx2, outy2);

	  dotted_mode (0);

	  /* draw the rectangle in the drawable region */
	  region = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS);
	  nrects = JI_REGION_NUM_RECTS(region);
	  for (c=0, rc=JI_REGION_RECTS(region);
	       c<nrects;
	       c++, rc++) {
	    set_clip(ji_screen, rc->x1, rc->y1, rc->x2-1, rc->y2-1);
	    rect(ji_screen, outx1, outy1, outx2, outy2, 0);
	  }
	  set_clip (ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);
	  jregion_free (region);

	  dotted_mode (-1);
	}
	/* for all other tools */
	else {
	  /* draw the changed area of the sprite */
	  outx1 = INT_MAX;
	  outy1 = INT_MAX;
	  outx2 = INT_MIN;
	  outy2 = INT_MIN;

	  /* draw the necessary dirty-portion of the sprite in the screen */
	  if (tool->flags & TOOL_UPDATE_ALL) {
	    outx1 = offset_x;
	    outy1 = offset_y;
	    outx2 = outx1 + editor->sprite->w-1;
	    outy2 = outy1 + editor->sprite->h-1;
	  }
	  else if (tool->flags & TOOL_UPDATE_POINT) {
	    outx1 = outx2 = x2;
	    outy1 = outy2 = y2;
	  }
	  else if (tool->flags & TOOL_UPDATE_TRACE) {
	    outx1 = MIN (x1, x2);
	    outy1 = MIN (y1, y2);
	    outx2 = MAX (x1, x2);
	    outy2 = MAX (y1, y2);
	  }
	  else if (tool->flags & TOOL_UPDATE_BOX) {
	    if (first_time) {
	      old_x1 = x1;
	      old_y1 = y1;
	      old_x2 = x2;
	      old_y2 = y2;
	    }

	    outx1 = MIN (MIN (x1, old_x1), MIN (x2, old_x2));
	    outy1 = MIN (MIN (y1, old_y1), MIN (y2, old_y2));
	    outx2 = MAX (MAX (x1, old_x1), MAX (x2, old_x2));
	    outy2 = MAX (MAX (y1, old_y1), MAX (y2, old_y2));

	    old_x1 = x1;
	    old_y1 = y1;
	    old_x2 = x2;
	    old_y2 = y2;
	  }
	  else if (tool->flags & TOOL_UPDATE_SPRAY) {
	    outx1 = x2-spray_width;
	    outy1 = y2-spray_width;
	    outx2 = x2+spray_width;
	    outy2 = y2+spray_width;
	  }
	  else if (tool->flags & TOOL_UPDATE_LAST4) {
	    for (c=0; c<4; c++) {
	      if (outx1 > pts[c*2  ]) outx1 = pts[c*2];
	      if (outx2 < pts[c*2  ]) outx2 = pts[c*2];
	      if (outy1 > pts[c*2+1]) outy1 = pts[c*2+1];
	      if (outy2 < pts[c*2+1]) outy2 = pts[c*2+1];
	    }
	  }

	  /* for non-tiled mode */
	  if (!tiled_mode) {
	    outx1 = MAX (outx1-brush->size/2-offset_x, 0);
	    outy1 = MAX (outy1-brush->size/2-offset_y, 0);
	    outx2 = MIN (outx2+brush->size/2-offset_x, editor->sprite->w-1);
	    outy2 = MIN (outy2+brush->size/2-offset_y, editor->sprite->h-1);
 
	    editors_draw_sprite (editor->sprite, outx1, outy1, outx2, outy2);
	  }
	  /* for tiled mode */
	  else {
	    outx1 = outx1-brush->size/2-offset_x;
	    outy1 = outy1-brush->size/2-offset_y;
	    outx2 = outx2+brush->size/2-offset_x;
	    outy2 = outy2+brush->size/2-offset_y;

#if 0 /* use this code to see what parts are updated  */
	    {
	      int x1, y1, x2, y2;
	      editor_to_screen(widget, 0, 0, &x1, &y1);
	      editor_to_screen(widget,
			       editor->sprite->w-1,
			       editor->sprite->h-1, &x2, &y2);
	      rectfill(ji_screen, x1, y1, x2, y2, 0);
	    }
#endif

	    editors_draw_sprite_tiled(editor->sprite, outx1, outy1, outx2, outy2);
	  }
	}
      }

      /* draw the cursor in the screen */
      editor_draw_cursor(widget, jmouse_x(0), jmouse_y(0));

      /* update the state-bar */
      if (jwidget_is_visible(status_bar)) {
	if (tool->flags & TOOL_UPDATE_BOX) {
	  char mode[256] = "";

	  if (current_tool == &ase_tool_marker) {
	    if (key_shifts & KB_ALT_FLAG) {
	      if (start_b & 1)
		strcat(mode, _("Replace"));
	      else
		strcat(mode, _("Intersect"));
	    }
	    else {
	      if (start_b & 1)
		strcat(mode, _("Union"));
	      else
		strcat(mode, _("Subtract"));
	    }
	  }

	  status_bar_set_text(status_bar, 0,
			      "%s %3d %3d %s %3d %3d (%s %3d %3d) %s",
			      _start, x1, y1,
			      _end, x2, y2,
			      _size, ABS (x2-x1)+1, ABS (y2-y1)+1, mode);
	}
	else {
	  status_bar_set_text(status_bar, 0, "%s %3d %3d", _pos, x1, y1);
	}

	jwidget_flush_redraw(status_bar);
	jmanager_dispatch_messages();
      }

      release_bitmap(ji_screen);
      forced_update = FALSE;
      first_time = FALSE;

      /* draw extra stuff */
      editor_draw_layer_boundary_safe(widget);
      editor_draw_grid_safe(widget);
      /* editor_draw_path_safe (widget, FALSE); */
    }

    /* draw mask */
    editor_draw_mask_safe(widget);

    /* spray updating process */
    if (current_tool == &ase_tool_spray) {
      if (ji_clock-spray_time > (JI_TICKS_PER_SEC/20)*(100-air_speed)/100) {
	spray_time = ji_clock;
	forced_update = TRUE;
      }
    }

    /* in CTRL or ALT changes update forced */
    if (tool->flags & TOOL_FIRST2LAST) {
      int new_shifts = key_shifts & (KB_SHIFT_FLAG |
				     KB_CTRL_FLAG | KB_ALT_FLAG);
      if (old_key_shifts != new_shifts) {
	old_key_shifts = new_shifts;
	forced_update = TRUE;
      }
    }

    gui_feedback();
  } while (editor_click(widget, &new_x, &new_y, &update,
			marker_scroll_callback));

  /* if we finished with the same button that we start began */
  if (!editor_click_cancel(widget)) {

    /* marker *******************************************************/
    if (tool == &ase_tool_marker) {
      void (*func) (Mask *, int, int, int, int);

      if (key_shifts & KB_ALT_FLAG) {
	if (start_b & 1)
	  func = mask_replace;
	else
	  func = mask_intersect;
      }
      else {
	if (start_b & 1)
	  func = mask_union;
	else
	  func = mask_subtract;
      }

      /* insert the undo operation */
      if (undo_is_enabled (editor->sprite->undo))
	undo_set_mask (editor->sprite->undo, editor->sprite);

      (*func) (editor->sprite->mask,
	       MIN (x1, x2) - offset_x,
	       MIN (y1, y2) - offset_y,
	       MAX (x1, x2) - MIN (x1, x2) + 1,
	       MAX (y1, y2) - MIN (y1, y2) + 1);

      sprite_generate_mask_boundaries(editor->sprite);
      update_screen_for_sprite(editor->sprite);
    }

    /* draw trace ***************************************************/
    else {
      /* insert the undo operation */
      if (undo_is_enabled(editor->sprite->undo))
	undo_dirty(editor->sprite->undo, dirty);

      update_screen_for_sprite(editor->sprite);
    }
  }
  else {
    /* restore the image's background */
    dirty_put(dirty);

    /* redraw */
    update_screen_for_sprite(editor->sprite);
  }

  editor_click_done(widget);

  /* destroy dirty structure */
  dirty_free(dirty);

  /* destroy rect-data used by the marker tool */
  if (rect_data) {
    rectrestore(rect_data);
    rectdiscard(rect_data);
    rect_data = NULL;
  }
}

/* draws the "tool" traces with the given points */
void do_tool_points(Sprite *sprite, Tool *tool, const char *_color,
		    int npoints, int *x, int *y)
{
  int x1=0, y1=0, x2=0, y2=0;
  Dirty *dirty = NULL;
  int c, i, pts[8];
  Image *image;
  int offset_x;
  int offset_y;
  int color;
  AlgoHLine hline_proc;
  int tiled = get_tiled_mode();

  /* get drawable information */
  image = GetImage2(sprite, &offset_x, &offset_y, NULL);

  /* we have a image layer to paint in? */
  /* error, the active layer is not visible */
  /* error, the active layer is locked */
  if (!image || !sprite->layer->readable || !sprite->layer->writable)
    return;

  /* select the hline procedure */
  hline_proc = drawmode_procs[MID (0, image->imgtype, 2)]
			     [MID (0, brush_mode, 2)];

  /* alignment offset */
  offset_x = -offset_x;
  offset_y = -offset_y;

  /* get the color to use for the image */
  color = get_color_for_image (image->imgtype, _color);

  /* global stuff needs */
  tool_image = image;
  tool_color = color;

  /* accumulative dirty */
  dirty = dirty_new(image, 0, 0, image->w-1, image->h-1, tiled);
  dirty->mask = (sprite->mask &&
		 sprite->mask->bitmap)? sprite->mask: NULL;

  for (c=0; c<npoints; c++) {
    /* common behavior for boxes likes tools */
    if (tool->flags & TOOL_FIRST2LAST) {
      if (c == 0)
	continue;

      x1 = x[0];
      y1 = y[0];
      x2 = x[c];
      y2 = y[c];
    }
    /* common behavior for pencil like tools */
    else if (tool->flags & TOOL_OLD2LAST) {
      x1 = (c == 0) ? x[0]: x[c-1];
      y1 = (c == 0) ? y[0]: y[c-1];
      x2 = x[c];
      y2 = y[c];
    }
    /* special behavior for brush */
    else if (tool->flags & TOOL_FOURCHAIN) {
      for (i=0; i<4; i++) {
	pts[i*2  ] = (c-3+i >= 0) ? x[c-3+i]: x[0];
	pts[i*2+1] = (c-3+i >= 0) ? y[c-3+i]: y[0];
      }
    }

    /* displace region */
/*     if (dirty->mask) */
/*       mask_move(dirty->mask, offset_x, offset_y); */

    /* create the area which the trace will dirty */
    if (tool == &ase_tool_brush) {
      algo_spline(pts[0], pts[1], pts[2], pts[3],
		  pts[4], pts[5], pts[6], pts[7],
		  dirty, fourchain_line);
    }
    else if (tool->put) {
      tool->put(dirty, x1, y1, x2, y2);
    }

    /* displace region */
/*     if (dirty->mask) */
/*       mask_move(dirty->mask, -offset_x, -offset_y); */

    /* get the background which the trace will overlap */
    dirty_get(dirty);

    /* draw the trace */
    algo_dirty(dirty, NULL, hline_proc);
  }

  /* marker *******************************************************/
  if (tool == &ase_tool_marker) {
  }

  /* draw trace ***************************************************/
  else {
    /* insert the undo operation */
    if (undo_is_enabled(sprite->undo))
      undo_dirty(sprite->undo, dirty);
  }

  /* destroy dirty structure */
  dirty_free(dirty);
}

void apply_grid(int *x, int *y)
{
  div_t d, dx, dy;
  int w = jrect_w(grid);
  int h = jrect_h(grid);

  dx = div(grid->x1, w);
  dy = div(grid->y1, h);

  d = div((*x)-dx.rem, w); *x = dx.rem + d.quot*w + ((d.rem > w/2)? w-1: 0);
  d = div((*y)-dy.rem, h); *y = dy.rem + d.quot*h + ((d.rem > h/2)? h-1: 0);
}

static void fourchain_line(int x1, int y1, int x2, int y2, void *data)
{
  if (brush->size == 1)
    dirty_line(data, x1, y1, x2, y2);
  else
    dirty_line_brush(data, brush, x1, y1, x2, y2);
}

/**********************************************************************/
/* Opaque draw mode						      */
/**********************************************************************/

static void my_image_hline4_opaque(int x1, int y, int x2, void *data)
{
  register ase_uint32 *address = ((ase_uint32 **)tool_image->line)[y]+x1;
  register int x = x2 - x1 + 1;
  int c = tool_color;

  while (x--)
    *(address++) = c;
}

static void my_image_hline2_opaque(int x1, int y, int x2, void *data)
{
  register ase_uint16 *address = ((ase_uint16 **)tool_image->line)[y]+x1;
  register int x = x2 - x1 + 1;
  int c = tool_color;

  while (x--)
    *(address++) = c;
}

static void my_image_hline1_opaque(int x1, int y, int x2, void *data)
{
  memset(((ase_uint8 **)tool_image->line)[y]+x1, tool_color, x2-x1+1);
}

/**********************************************************************/
/* Glass draw mode						      */
/**********************************************************************/

static void my_image_hline4_glass(int x1, int y, int x2, void *data)
{
  register ase_uint32 *address = ((ase_uint32 **)tool_image->line)[y]+x1;
  register int x = x2 - x1 + 1;
  int c = _rgba (_rgba_getr (tool_color),
		 _rgba_getg (tool_color),
		 _rgba_getb (tool_color), glass_dirty);
  int o = _rgba_geta (tool_color);

  while (x--) {
    *address = _rgba_blend_normal (*address, c, o);
    address++;
  }
}

static void my_image_hline2_glass(int x1, int y, int x2, void *data)
{
  register ase_uint16 *address = ((ase_uint16 **)tool_image->line)[y]+x1;
  register int x = x2 - x1 + 1;
  int c = _graya (_graya_getk (tool_color), glass_dirty);
  int o = _graya_geta (tool_color);

  while (x--) {
    *address = _graya_blend_normal (*address, c, o);
    address++;
  }
}

static void my_image_hline1_glass(int x1, int y, int x2, void *data)
{
  register ase_uint8 *address = ((ase_uint8 **)tool_image->line)[y]+x1;
  register int x = x2 - x1 + 1;
  int c, tc = _rgba(_rgb_scale_6[_current_palette[_index_cmap[tool_color]].r],
		    _rgb_scale_6[_current_palette[_index_cmap[tool_color]].g],
		    _rgb_scale_6[_current_palette[_index_cmap[tool_color]].b], 255);

  while (x--) {
    c = _rgba_blend_normal(_rgba(_rgb_scale_6[_current_palette[*address].r],
				 _rgb_scale_6[_current_palette[*address].g],
				 _rgb_scale_6[_current_palette[*address].b], 255),
			   tc, glass_dirty);

/*     *(address++) = rgb_map->data[_rgba_getr (c)>>3] */
/* 				[_rgba_getg (c)>>3] */
/* 				[_rgba_getb (c)>>3]; */
    *address = orig_rgb_map->data
      [_rgba_getr (c)>>3]
      [_rgba_getg (c)>>3]
      [_rgba_getb (c)>>3];
    address++;
  }
}

/**********************************************************************/
/* Semi draw mode						      */
/**********************************************************************/

static void my_image_hline4_semi(int x1, int y, int x2, void *data)
{
  register ase_uint32 *address = ((ase_uint32 **)tool_image->line)[y]+x1;
  register int x;

  for (x=x1; x<=x2; x++) {
    if ((x+y)&1)
      *address = tool_color;
    address++;
  }
}

static void my_image_hline2_semi(int x1, int y, int x2, void *data)
{
  register ase_uint16 *address = ((ase_uint16 **)tool_image->line)[y]+x1;
  register int x;

  for (x=x1; x<=x2; x++) {
    if ((x+y)&1)
      *address = tool_color;
    address++;
  }
}

static void my_image_hline1_semi(int x1, int y, int x2, void *data)
{
  register ase_uint8 *address = ((ase_uint8 **)tool_image->line)[y]+x1;
  register int x;

  for (x=x1; x<=x2; x++) {
    if ((x+y)&1)
      *address = tool_color;
    address++;
  }
}
