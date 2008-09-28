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
#include <limits.h>
#include <math.h>

#include "jinete/jaccel.h"
#include "jinete/jalert.h"
#include "jinete/jlist.h"
#include "jinete/jmanager.h"
#include "jinete/jmessage.h"
#include "jinete/jregion.h"
#include "jinete/jsystem.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"

#include "core/app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "effect/effect.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/sprites.h"
#include "modules/tools.h"
#include "raster/raster.h"
#include "util/misc.h"
#include "util/render.h"
#include "widgets/colbar.h"
#include "widgets/editor.h"
#include "widgets/statebar.h"
#include "widgets/toolbar.h"

/* tool flags */
#define TOOL_COPY_SRC2DST       0x00000001
#define TOOL_COPY_DST2SRC       0x00000002
#define TOOL_FIRST2LAST		0x00000004 /* for box like tools (line,ellipse,rectangle) */
#define TOOL_OLD2LAST		0x00000008 /* for free-hand tools (pencil) */
#define TOOL_4OLD2LAST          0x00000010 /* for free-hand tools (4 points, brush) */
#define TOOL_ACCEPT_FILL	0x00000020
#define TOOL_UPDATE_ALL		0x00000040
#define TOOL_UPDATE_POINT	0x00000080
#define TOOL_UPDATE_TRACE	0x00000100
#define TOOL_UPDATE_BOX		0x00000200
#define TOOL_UPDATE_SPRAY	0x00000400
#define TOOL_UPDATE_4PTS	0x00000800
#define TOOL_SNAP_ANGLES	0x00001000
#define TOOL_4FIRST2LAST	0x00002000 /* for bezier like tools (4 points, curve) */

Tool *current_tool = NULL;

static Brush *brush = NULL;
static int glass_dirty;
static int spray_width;
static int air_speed;
static bool filled_mode;
static bool tiled_mode;
static bool use_grid;
static bool view_grid;
static JRect grid;
static bool onionskin;

static color_t cursor_color;
static int _cursor_color;
static int _cursor_mask;

/***********************************************************/
/* INKS                                                    */
/***********************************************************/

enum {
  INK_OPAQUE,
  INK_GLASS,
  INK_SOFTEN,
  INK_REPLACE,
  INK_JUMBLE,
  MAX_INKS
};

static void ink_hline4_opaque(int x1, int y, int x2, ToolData *data);
static void ink_hline2_opaque(int x1, int y, int x2, ToolData *data);
static void ink_hline1_opaque(int x1, int y, int x2, ToolData *data);

static void ink_hline4_glass(int x1, int y, int x2, ToolData *data);
static void ink_hline2_glass(int x1, int y, int x2, ToolData *data);
static void ink_hline1_glass(int x1, int y, int x2, ToolData *data);

static void ink_hline4_soften(int x1, int y, int x2, ToolData *data);
static void ink_hline2_soften(int x1, int y, int x2, ToolData *data);
static void ink_hline1_soften(int x1, int y, int x2, ToolData *data);

static void ink_hline4_replace(int x1, int y, int x2, ToolData *data);
static void ink_hline2_replace(int x1, int y, int x2, ToolData *data);
static void ink_hline1_replace(int x1, int y, int x2, ToolData *data);

static void ink_hline4_jumble(int x1, int y, int x2, ToolData *data);
static void ink_hline2_jumble(int x1, int y, int x2, ToolData *data);
static void ink_hline1_jumble(int x1, int y, int x2, ToolData *data);

static AlgoHLine inks_hline[][3] =
{
#define DEF_INK(name)			\
  { (AlgoHLine)ink_hline4_##name,	\
    (AlgoHLine)ink_hline2_##name,	\
    (AlgoHLine)ink_hline1_##name }

  DEF_INK(opaque),
  DEF_INK(glass),
  DEF_INK(soften),
  DEF_INK(replace),
  DEF_INK(jumble)
};

/***********************************************************/
/* CURSOR COLOR                                            */
/***********************************************************/

static void update_cursor_color(void *data)
{
  if (ji_screen)
    _cursor_color = get_color_for_allegro(bitmap_color_depth(ji_screen),
					  cursor_color);
  else
    _cursor_color = 0;

  _cursor_mask = (color_type(cursor_color) == COLOR_TYPE_MASK);
}

/***********************************************************/
/* TOOLS                                                   */
/***********************************************************/

int init_module_tools(void)
{
  int size, type, angle;

  /* current tool */
  current_tool = tools_list[TOOL_PENCIL];

  /* brush */
  brush = brush_new();

  type = get_config_int("Tools", "BrushType", BRUSH_CIRCLE);
  size = get_config_int("Tools", "BrushSize", 1);
  angle = get_config_int("Tools", "BrushAngle", 0);

  brush_set_type(brush, MID(BRUSH_CIRCLE, type, BRUSH_LINE));
  brush_set_size(brush, MID(1, size, 32));
  brush_set_angle(brush, MID(0, angle, 360));

  /* cursor color */
  set_cursor_color(get_config_color("Tools", "CursorColor", color_mask()));

  /* tools configuration */
  glass_dirty = get_config_int("Tools", "GlassDirty", 255);
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

  app_add_hook(APP_PALETTE_CHANGE, update_cursor_color, NULL);

  return 0;
}

void exit_module_tools(void)
{
  int c;

  set_config_color("Tools", "CursorColor", cursor_color);
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

  jrect_free(grid);
  brush_free(brush);

  for (c=0; c<MAX_TOOLS; c++) {
    if (tools_list[c]->accel) {
      jaccel_free(tools_list[c]->accel);
      tools_list[c]->accel = NULL;
    }
  }
}

void tool_add_key(Tool *tool, const char *string)
{
  char buf[256];

  if (!tool->accel)
    tool->accel = jaccel_new();

  usprintf(buf, "<%s>", string);
  jaccel_add_keys_from_string(tool->accel, buf);
}

bool tool_is_key_pressed(Tool *tool, JMessage msg)
{
  if (tool->accel) {
    return jaccel_check(tool->accel,
			msg->any.shifts,
			msg->key.ascii,
			msg->key.scancode);
  }
  return FALSE;
}

Tool *get_tool_by_name(const char *name)
{
  int c;

  for (c=0; c<MAX_TOOLS; c++)
    if (ustrcmp(tools_list[c]->key, name) == 0)
      return tools_list[c];

  return NULL;
}

Tool *get_tool_by_key(JMessage msg)
{
  Tool *group[MAX_TOOLS];
  int i, j;

  for (i=j=0; i<MAX_TOOLS; i++) {
    if (tool_is_key_pressed(tools_list[i], msg))
      group[j++] = tools_list[i];
  }

  if (j == 0)
    return NULL;

  for (i=0; i<j; i++)
    if (group[i] == current_tool)
      if (i+1 < j)
	return group[i+1];

  return group[0];
}

void select_tool(Tool *tool)
{
  assert(tool != NULL);
  
  current_tool = tool;

  /* update status-bar */
  if (app_get_statusbar() &&
      jwidget_is_visible(app_get_statusbar()))
    statusbar_set_text(app_get_statusbar(), 500, "%s: %s",
		       _("Tool"), _(current_tool->name));

  /* update tool-bar */
  if (app_get_toolbar())
    toolbar_update(app_get_toolbar());
}

Brush *get_brush(void) { return brush; }
int get_brush_type(void) { return brush->type; }
int get_brush_size(void) { return brush->size; }
int get_brush_angle(void) { return brush->angle; }
int get_glass_dirty(void) { return glass_dirty; }
int get_spray_width(void) { return spray_width; }
int get_air_speed(void) { return air_speed; }
bool get_filled_mode(void) { return filled_mode; }
bool get_tiled_mode(void) { return tiled_mode; }
bool get_use_grid(void) { return use_grid; }
bool get_view_grid(void) { return view_grid; }

JRect get_grid(void)
{
  return jrect_new_copy(grid);
}

bool get_onionskin(void)
{
  return onionskin;
}

void set_brush_type(int type) { brush_set_type(brush, type); }
void set_brush_size(int size) { brush_set_size(brush, size); }
void set_brush_angle(int angle) { brush_set_angle(brush, angle); }
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

color_t get_cursor_color(void)
{
  return cursor_color;
}

void set_cursor_color(color_t color)
{
  cursor_color = color;
  update_cursor_color(NULL);
}

/* returns the size which use the current tool */
int get_thickness_for_cursor(void)
{
  /* 1 pixel of thickness */
  if ((current_tool == tools_list[TOOL_MARKER]) ||
      (current_tool == tools_list[TOOL_FLOODFILL]))
    return 1;
  /* the spray have a special thickness (for the spray-width) */
/*   else if (current_tool == &ase_tool_spray) */
/*     return brush->size+spray_width*2; */
  /* all the other tools use the original thickness */
  else
    return brush->size;
}

/***********************************************************/
/* BRIDGE BETWEEN TOOLS -> INKS                            */
/***********************************************************/

static void do_ink_pixel(int x, int y, ToolData *data)
{
  /* tiled mode */
  if (data->tiled) {
    register int size;

    size = data->dst_image->w;
    if (x < 0)
      x = size - (-(x+1) % size) - 1;
    else
      x = x % size;

    size = data->dst_image->h;
    if (y < 0)
      y = size - (-(y+1) % size) - 1;
    else
      y = y % size;
  }
  /* clipped */
  else {
    if (y < 0 || y >= data->dst_image->h ||
	x < 0 || x >= data->dst_image->w)
      return;
  }

  data->ink_hline_proc(x, y, x, data);
}

static void do_ink_hline(int x1, int y, int x2, ToolData *data)
{
  /* tiled mode */
  if (data->tiled) {
    register int w, size;	/* width or height */
    register int x;

    if (x1 > x2)
      return;

    size = data->dst_image->h;	/* size = image height */
    if (y < 0)
      y = size - (-(y+1) % size) - 1;
    else
      y = y % size;

    size = data->dst_image->w;	/* size = image width */
    w = x2-x1+1;
    if (w >= size)
      data->ink_hline_proc(0, y, size-1, data);
    else {
      x = x1;
      if (x < 0)
	x = size - (-(x+1) % size) - 1;
      else
	x = x % size;

      if (x+w-1 <= size-1)
	data->ink_hline_proc(x, y, x+w-1, data);
      else {
	data->ink_hline_proc(x, y, size-1, data);
	data->ink_hline_proc(0, y, w-(size-x)-1, data);
      }
    }
  }
  /* clipped */
  else {
    if (y < 0 || y >= data->dst_image->h)
      return;

    if (x1 < 0)
      x1 = 0;

    if (x2 >= data->dst_image->w)
      x2 = data->dst_image->w-1;

    if (x2-x1+1 < 1)
      return;

    data->ink_hline_proc(x1, y, x2, data);
  }
}

static void do_ink_brush(int x, int y, ToolData *data)
{
  register struct BrushScanline *scanline = data->brush->scanline;
  register int h = data->brush->size;
  register int c = h/2;

  x -= c;
  y -= c;

  for (c=0; c<h; c++) {
    if (scanline->state)
      do_ink_hline(x+scanline->x1, y+c, x+scanline->x2, data);
    ++scanline;
  }
}

/***********************************************************/
/* MARKER                                                  */
/***********************************************************/

static Tool tool_marker =
{
  "rectangular_marquee",
  "Rectangular Marquee Tool",
  "Rectangular Marquee Tool",
  TOOL_FIRST2LAST | TOOL_UPDATE_BOX,
  NULL
};

/***********************************************************/
/* PENCIL                                                  */
/***********************************************************/

static void tool_pencil_draw_trace(int x1, int y1, int x2, int y2, ToolData *data)
{
  algo_line(x1, y1, x2, y2, data,
	    (data->brush->size == 1) ?
	    (AlgoPixel)do_ink_pixel:
	    (AlgoPixel)do_ink_brush);
}

static Tool tool_pencil =
{
  "pencil",
  "Pencil Tool",
  "Pencil Tool",
  TOOL_OLD2LAST | TOOL_UPDATE_TRACE,
  NULL,
  tool_pencil_draw_trace,
  NULL
};

/***********************************************************/
/* BRUSH                                                   */
/***********************************************************/

static Tool tool_brush =
{
  "brush",
  "Brush Tool",
  "Brush Tool",
  TOOL_4OLD2LAST | TOOL_UPDATE_4PTS,
  NULL,
  NULL
};

/***********************************************************/
/* ERASER                                                  */
/***********************************************************/

static void tool_eraser_preprocess_data(ToolData *data)
{
  /* left button: glass ink */
  if (data->left_button) {
    data->color = app_get_color_to_clear_layer(data->layer);

    data->ink_hline_proc =
      inks_hline[INK_OPAQUE][MID(0, data->dst_image->imgtype, 2)];
  }
  /* right button: replace ink */
  else {
    data->ink_hline_proc =
      inks_hline[INK_REPLACE][MID(0, data->dst_image->imgtype, 2)];
  }
}

static void tool_eraser_draw_trace(int x1, int y1, int x2, int y2, ToolData *data)
{
  algo_line(x1, y1, x2, y2, data,
	    (brush->size == 1)?
	    (AlgoPixel)do_ink_pixel:
	    (AlgoPixel)do_ink_brush);
}

static Tool tool_eraser =
{
  "eraser",
  "Eraser Tool",
  "Eraser Tool:\n"
  "* Left-button: Erase with the background color in `Background' layer \n"
  "               or transparent color in any other layer.\n"
  "* Right-button: Replace foreground with background color.",
  TOOL_OLD2LAST | TOOL_UPDATE_TRACE,
  tool_eraser_preprocess_data,
  tool_eraser_draw_trace,
  NULL
};

/***********************************************************/
/* FLOODFILL                                               */
/***********************************************************/

static void tool_floodfill_draw_trace(int x1, int y1, int x2, int y2, ToolData *data)
{
  if (image_getpixel(data->dst_image, x2, y2) != data->color) {
    algo_floodfill(data->dst_image, x2, y2, data,
		   (AlgoHLine)do_ink_hline);
  }
}

static Tool tool_floodfill =
{
  "paint_bucket",
  "Paint Bucket Tool",
  "Paint Bucket Tool",
  TOOL_OLD2LAST | TOOL_UPDATE_ALL,
  NULL,
  tool_floodfill_draw_trace,
  NULL
};

/***********************************************************/
/* SPRAY                                                   */
/***********************************************************/

static void tool_spray_draw_trace(int x1, int y1, int x2, int y2, ToolData *data)
{
  int c, x, y, times = (spray_width*spray_width/4) * air_speed / 100;

#ifdef __MINGW32__             /* MinGW32 has a RAND_MAX too small */
  fixed angle, radius;

  for (c=0; c<times; c++) {
    angle = itofix(rand() * 256 / RAND_MAX);
    radius = itofix(rand() * (spray_width*10) / RAND_MAX) / 10;
    x = fixtoi(fixmul(radius, fixcos(angle)));
    y = fixtoi(fixmul(radius, fixsin(angle)));
    do_ink_brush(x2+x, y2+y, data);
  }
#else
  fixed angle, radius;

  for (c=0; c<times; c++) {
    angle = rand();
    radius = rand() % itofix(spray_width);
    x = fixtoi(fixmul(radius, fixcos(angle)));
    y = fixtoi(fixmul(radius, fixsin(angle)));
    do_ink_brush(x2+x, y2+y, data);
  }
#endif
}

static Tool tool_spray =
{
  "spray",
  "Spray Tool",
  "Spray Tool",
  TOOL_COPY_DST2SRC | TOOL_OLD2LAST | TOOL_UPDATE_SPRAY,
  NULL,
  tool_spray_draw_trace,
  NULL
};

/***********************************************************/
/* LINE                                                    */
/***********************************************************/

static void tool_line_draw_trace(int x1, int y1, int x2, int y2, ToolData *data)
{
  algo_line(x1, y1, x2, y2, data,
	    (brush->size == 1) ?
	    (AlgoPixel)do_ink_pixel:
	    (AlgoPixel)do_ink_brush);
}

static Tool tool_line =
{
  "line",
  "Line Tool",
  "Line Tool",
  TOOL_COPY_SRC2DST | TOOL_FIRST2LAST | TOOL_UPDATE_BOX | TOOL_SNAP_ANGLES,
  NULL,
  tool_line_draw_trace,
  NULL
};

/***********************************************************/
/* CURVE                                                   */
/***********************************************************/

static void tool_curve_draw_trace(int x1, int y1, int x2, int y2, ToolData *data)
{
  algo_line(x1, y1, x2, y2, data,
	    (brush->size == 1) ?
	    (AlgoPixel)do_ink_pixel:
	    (AlgoPixel)do_ink_brush);
}

static Tool tool_curve =
{
  "curve",
  "Curve Tool",
  "Curve Tool",
  TOOL_COPY_SRC2DST | TOOL_4FIRST2LAST | TOOL_UPDATE_4PTS | TOOL_SNAP_ANGLES,
  NULL,
  tool_curve_draw_trace,
  NULL
};

/***********************************************************/
/* RECTANGLE                                               */
/***********************************************************/

static void tool_rectangle_draw_trace(int x1, int y1, int x2, int y2, ToolData *data)
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

  if (filled_mode) {
    int bak_color;
    bak_color = data->color;
    data->color = data->other_color;

    for (c=y1; c<=y2; c++)
      do_ink_hline(x1, c, x2, data);

    data->color = bak_color;
  }

  for (c=x1; c<=x2; c++) do_ink_brush(c, y1, data);
  for (c=x1; c<=x2; c++) do_ink_brush(c, y2, data);
  for (c=y1; c<=y2; c++) {
    do_ink_brush(x1, c, data);
    do_ink_brush(x2, c, data);
  }
}

static Tool tool_rectangle =
{
  "rectangle",
  "Rectangle Tool",
  "Rectangle Tool",
  TOOL_COPY_SRC2DST | TOOL_FIRST2LAST | TOOL_UPDATE_BOX | TOOL_ACCEPT_FILL,
  NULL,
  tool_rectangle_draw_trace,
  NULL
};

/***********************************************************/
/* ELLIPSE                                                 */
/***********************************************************/

static void tool_ellipse_draw_trace(int x1, int y1, int x2, int y2, ToolData *data)
{
  if (filled_mode) {
    int bak_color;
    bak_color = data->color;
    data->color = data->other_color;

    algo_ellipsefill(x1, y1, x2, y2, data,
		     (AlgoHLine)do_ink_hline);

    data->color = bak_color;
  }

  algo_ellipse(x1, y1, x2, y2, data,
	       (AlgoPixel)do_ink_brush);
}

static Tool tool_ellipse =
{
  "ellipse",
  "Ellipse Tool",
  "Ellipse Tool",
  TOOL_COPY_SRC2DST | TOOL_FIRST2LAST | TOOL_UPDATE_BOX | TOOL_ACCEPT_FILL,
  NULL,
  tool_ellipse_draw_trace,
  NULL
};

/***********************************************************/
/* BLUR                                                    */
/***********************************************************/

static void tool_blur_preprocess_data(ToolData *data)
{
  data->ink_hline_proc =
    inks_hline[INK_SOFTEN][MID(0, data->dst_image->imgtype, 2)];
}

static void tool_blur_draw_trace(int x1, int y1, int x2, int y2, ToolData *data)
{
  algo_line(x1, y1, x2, y2, data,
	    (data->brush->size == 1) ?
	    (AlgoPixel)do_ink_pixel:
	    (AlgoPixel)do_ink_brush);
}

static Tool tool_blur =
{
  "blur",
  "Blur Tool",
  "Blur Tool",
  TOOL_COPY_DST2SRC | TOOL_OLD2LAST | TOOL_UPDATE_TRACE,
  tool_blur_preprocess_data,
  tool_blur_draw_trace,
  NULL
};

/***********************************************************/
/* JUMBLE                                                  */
/***********************************************************/

static void tool_jumble_preprocess_data(ToolData *data)
{
  data->ink_hline_proc =
    inks_hline[INK_JUMBLE][MID(0, data->dst_image->imgtype, 2)];
}

static void tool_jumble_draw_trace(int x1, int y1, int x2, int y2, ToolData *data)
{
  algo_line(x1, y1, x2, y2, data,
	    (data->brush->size == 1) ?
	    (AlgoPixel)do_ink_pixel:
	    (AlgoPixel)do_ink_brush);
}

static Tool tool_jumble =
{
  "jumble",
  "Jumble Tool",
  "Jumble Tool",
  TOOL_COPY_DST2SRC | TOOL_OLD2LAST | TOOL_UPDATE_TRACE,
  tool_jumble_preprocess_data,
  tool_jumble_draw_trace,
  NULL
};

/***********************************************************/
/* TOOL'S LIST                                             */
/***********************************************************/

/* at the same order as TOOL_* enumeration */
Tool *tools_list[MAX_TOOLS] =
{
  &tool_marker,
  &tool_pencil,
  &tool_brush,
  &tool_eraser,
  &tool_floodfill,
  &tool_spray,
  &tool_line,
  &tool_curve,
  &tool_rectangle,
  &tool_ellipse,
  &tool_blur,
  &tool_jumble
};

/***********************************************************/
/* TOOL CONTROL                                            */
/***********************************************************/

static void *rect_tracker = NULL;

static void line_for_spline(int x1, int y1, int x2, int y2, ToolData *data);

static void marker_scroll_callback(int before_change)
{
  if (before_change && rect_tracker) {
    rect_tracker_free(rect_tracker);
    rect_tracker = NULL;
  }
}

/* controls any tool to draw in the current sprite */
void control_tool(JWidget widget, Tool *tool,
		  color_t _color,
		  color_t _other_color,
		  bool left_button)
{
  Editor *editor = editor_data(widget);
  Sprite *sprite = editor->sprite;
  JWidget statusbar = app_get_statusbar();
  int x1, y1, x2, y2;
  int old_x1, old_y1, old_x2, old_y2;
  int outx1, outy1, outx2, outy2;
  int mouse_x[4];
  int mouse_y[4];
  int c, pts[8], old_pts[8];
  int start_x, new_x, offset_x, old_cel_x;
  int start_y, new_y, offset_y, old_cel_y;
  int start_b;
  int old_key_shifts, update, forced_update, first_time;
  int spray_time;
  const char *_pos = _("Pos");
  const char *_start = _("Start");
  const char *_end = _("End");
  const char *_size = _("Size");
  bool click2 = get_config_bool("Options", "DrawClick2", FALSE);
  Cel *cel = NULL;
  Image *cel_image = NULL;
  bool cel_created = FALSE;
  bool destroy_cel = FALSE;
  int curve_pts = 0;	   /* to iterate points in the 'curve' tool */
  ToolData tool_data;

  /* First of all we have to dispatch the enqueue messages. Why is it
     needed?  To dispatch the JM_CLOSE and redrawing messages if a
     color-selector was shown to the user before to click. This is
     because the tooltips filter the JM_BUTTONPRESSED message, so when
     the user click outside the window, it is closed, but the filtered
     JM_BUTTONPRESSED message is not "eaten" so the editor receive it
     anyway */
  jmanager_dispatch_messages(jwidget_get_manager(widget));
  jwidget_flush_redraw(jwidget_get_manager(widget));

  /* error, the active layer is not visible */
  if (!layer_is_readable(sprite->layer)) {
    jalert(_(PACKAGE
	     "<<The current layer is hidden,"
	     "<<make it visible and try again"
	     "||&Close"));
    return;
  }
  /* error, the active layer is locked */
  else if (!layer_is_writable(sprite->layer)) {
    jalert(_(PACKAGE
	     "<<The current layer is locked,"
	     "<<unlock it and try again"
	     "||&Close"));
    return;
  }

  /* get cel and image where we can draw */
  cel_image = NULL;

  if (sprite != NULL &&
      sprite->layer != NULL &&
      layer_is_image(sprite->layer)) {
    cel = layer_get_cel(sprite->layer,
			sprite->frame);
    if (cel != NULL)
      cel_image = sprite->stock->image[cel->image];
  }

  if (cel == NULL) {
    /* int image_index; */

    /* create the image */
    cel_image = image_new(sprite->imgtype, sprite->w, sprite->h);
    image_clear(cel_image, 0);

    /* add it to the stock */
    /* image_index = stock_add_image(sprite->stock, cel_image); */

    /* create the cel */
    cel = cel_new(sprite->frame, 0);
    layer_add_cel(sprite->layer, cel);

    cel_created = TRUE;
  }

  /* /\* isn't a cel in the current frame/layer, we create a new one *\/ */
  /* if (cel == NULL) { */
  /*   cel_created = TRUE; */
  /*   cel = cel_new(sprite->frame, -1); */
  /* } */

  /* /\* if we don't have an image to paint in, we create new image to paint in *\/ */
  /* if (cel_image == NULL) { */
  /*   cel_image_created = TRUE; */
  /* } */
  /* /\* if the cel is linked we make a copy of it (so it can be unlinked) *\/ */
  /* else if (cel_is_link(cel, sprite->layer)) { */
  /*   cel_image_created = TRUE; */
  /*   cel_image = image_new_copy(cel_image); */
  /* } */

  old_cel_x = cel->x;
  old_cel_y = cel->y;

  /* prepare the ToolData... */
  tool_data.sprite = sprite;
  tool_data.layer = sprite->layer;
  tool_data.tiled = tiled_mode;

  if (!tool_data.tiled) {	/* not tiled mode */
    x1 = MIN(cel->x, 0);
    y1 = MIN(cel->y, 0);
    x2 = MAX(cel->x+cel_image->w, sprite->w);
    y2 = MAX(cel->y+cel_image->h, sprite->h);
  }
  else {			/* tiled mode */
    x1 = 0;
    y1 = 0;
    x2 = sprite->w;
    y2 = sprite->h;
  }

  tool_data.src_image = image_crop(cel_image,
				   x1-cel->x,
				   y1-cel->y, x2-x1, y2-y1, 0);
  tool_data.dst_image = image_new_copy(tool_data.src_image);

  tool_data.mask = (sprite->mask &&
		    sprite->mask->bitmap)? sprite->mask: NULL;
  tool_data.mask_x = tool_data.mask ? tool_data.mask->x-x1: 0;
  tool_data.mask_y = tool_data.mask ? tool_data.mask->y-y1: 0;

  tool_data.brush = brush;
  tool_data.color = get_color_for_layer(sprite->layer, _color);
  tool_data.other_color = get_color_for_layer(sprite->layer, _other_color);
  tool_data.left_button = left_button;
  tool_data.opacity = glass_dirty;
  tool_data.vector.x = 0;
  tool_data.vector.y = 0;

  tool_data.ink_hline_proc =
    tool_data.opacity == 255 ? inks_hline[INK_OPAQUE][MID(0, cel_image->imgtype, 2)]:
			       inks_hline[INK_GLASS][MID(0, cel_image->imgtype, 2)];

  if (tool->preprocess_data)
    tool->preprocess_data(&tool_data);

  /* the 'tool_image' should be used to draw the `sprite->layer' in
     the `render_sprite' routine */
  set_preview_image(sprite->layer, tool_data.dst_image);

  /* we have to modify the cel position because it's used in the
     `render_sprite' routine to draw the `dst_image' */
  cel->x = x1;
  cel->y = y1;
  offset_x = -x1;
  offset_y = -y1;
  
  update = FALSE;
  forced_update = TRUE;
  first_time = TRUE;
  old_key_shifts = key_shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG);
  spray_time = ji_clock;
  old_x1 = old_y1 = old_x2 = old_y2 = 0;

next_pts:;

  /* start click */
  editor_click_start(widget,
		     click2 ? MODE_CLICKANDCLICK:
			      MODE_CLICKANDRELEASE,
		     &start_x, &start_y, &start_b);

  for (c=0; c<4; c++) {
    mouse_x[c] = start_x;
    mouse_y[c] = start_y;
  }

  if (tool->flags & TOOL_4FIRST2LAST &&
      curve_pts == 0) {
    x1 = mouse_x[0];
    y1 = mouse_y[0];

    if (use_grid)
      apply_grid(&x1, &y1, TRUE);

    x1 += offset_x;
    y1 += offset_y;

    pts[0] = pts[2] = pts[4] = pts[6] = x1;
    pts[1] = pts[3] = pts[5] = pts[7] = y1;
    ++curve_pts;
  }

  do {
    if ((update) || (forced_update)) {
      int real_update =
	(current_tool != tools_list[TOOL_SPRAY]) ||
	(forced_update);

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
	  apply_grid(&x1, &y1, TRUE);
	  apply_grid(&x2, &y2, TRUE);
        }

	/* square aspect */
	if (old_key_shifts & KB_SHIFT_FLAG) {
	  int dx = x2 - x1;
	  int dy = y2 - y1;
	  int size;

	  if (tool->flags & TOOL_SNAP_ANGLES)
	    size = MAX(ABS(dx), ABS(dy));
	  else
	    size = MIN(ABS(dx), ABS(dy));

	  x2 = x1 + SGN(dx) * size;
	  y2 = y1 + SGN(dy) * size;

	  if (tool->flags & TOOL_SNAP_ANGLES) {
	    if (ABS(dx) <= ABS(dy)/2)
	      x2 = x1;
	    else if (ABS(dy) <= ABS(dx)/2)
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

	tool_data.vector.x = x2 - x1;
	tool_data.vector.y = y2 - y1;
      }
      /* common behavior for pencil like tools */
      else if (tool->flags & TOOL_OLD2LAST) {
	x1 = mouse_x[1];
	y1 = mouse_y[1];
	x2 = mouse_x[0];
	y2 = mouse_y[0];

	/* grid */
        if (use_grid && (tool != tools_list[TOOL_FLOODFILL])) {
	  apply_grid(&x1, &y1, TRUE);
	  apply_grid(&x2, &y2, TRUE);
        }

	x1 += offset_x;
	y1 += offset_y;
	x2 += offset_x;
	y2 += offset_y;

	tool_data.vector.x = x2 - x1;
	tool_data.vector.y = y2 - y1;
      }
      /* special behavior for brush */
      else if (tool->flags & TOOL_4OLD2LAST) {
        for (c=0; c<4; c++) {
	  pts[c*2  ] = mouse_x[c];
	  pts[c*2+1] = mouse_y[c];

	  /* grid */
	  if (use_grid)
	    apply_grid(&pts[c*2], &pts[c*2+1], TRUE);

	  pts[c*2  ] += offset_x;
	  pts[c*2+1] += offset_y;
        }

        x1 = pts[0];
        y1 = pts[1];
      }
      /* special behavior for curve */
      else if (tool->flags & TOOL_4FIRST2LAST) {
	x1 = mouse_x[0];
	y1 = mouse_y[0];

        if (use_grid)
	  apply_grid(&x1, &y1, TRUE);

	x1 += offset_x;
	y1 += offset_y;

	for (c=0; c<8; ++c)
	  old_pts[c] = pts[c];

	switch (curve_pts) {
	  case 1:
	    pts[2] = pts[4] = pts[6] = x1;
	    pts[3] = pts[5] = pts[7] = y1;
	    break;
	  case 2:
	    pts[2] = x1;
	    pts[3] = y1;
	    break;
	  case 3:
	    pts[4] = x1;
	    pts[5] = y1;
	    break;
	}
      }
      else {
        x1 = y1 = x2 = y2 = 0;
      }

      if (real_update) {
	if (tool->flags & TOOL_COPY_SRC2DST) {
	  image_clear(tool_data.dst_image, 0);
	  image_copy(tool_data.dst_image, tool_data.src_image, 0, 0);
	}
	else if (tool->flags & TOOL_COPY_DST2SRC) {
	  image_copy(tool_data.src_image, tool_data.dst_image, 0, 0);
	}

	/* displace region */
/* 	if (dirty->mask) */
/* 	  mask_move(dirty->mask, offset_x, offset_y); */

	/* create the area which the trace will dirty */
	if (tool == tools_list[TOOL_BRUSH] ||
	    tool == tools_list[TOOL_CURVE]) {
	  algo_spline(pts[0], pts[1], pts[2], pts[3],
		      pts[4], pts[5], pts[6], pts[7],
		      &tool_data, (AlgoLine)line_for_spline);
	}
	else if (tool->draw_trace) {
	  tool->draw_trace(x1, y1, x2, y2, &tool_data);
	}

	/* displace region */
/* 	if (dirty->mask) */
/* 	  mask_move(dirty->mask, -offset_x, -offset_y); */

	/* get the background which the trace will overlap */
/* 	dirty_save_image_data(dirty); */

	/* draw the trace */
/* 	algo_dirty(dirty, NULL, hline_proc); */
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

/* 	  if (!sprite->path) { */
/* 	    Path *path = path_new("*working*"); */
/* 	    sprite_set_path(sprite, path); */
/* 	  } */
/* 	  else { */
/* 	    PathNode *last = jlist_last(sprite->path->nodes)->data; */
/* 	    last->n = node; */
/* 	    node->p = last; */
/* 	  } */

/* 	  path_append_node(sprite->path, node); */
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
	if (tool == tools_list[TOOL_MARKER]) {
	  /* draw the rectangle mark */
	  JRegion region;
	  int nrects;
	  JRect rc;

	  editor_to_screen(widget,
			   MIN(x1, x2)-offset_x,
			   MIN(y1, y2)-offset_y, &outx1, &outy1);

	  editor_to_screen(widget,
			   MAX(x1, x2)-offset_x,
			   MAX(y1, y2)-offset_y, &outx2, &outy2);

	  outx2 += (1<<editor->zoom)-1;
	  outy2 += (1<<editor->zoom)-1;

	  if (rect_tracker)
	    rect_tracker_free(rect_tracker);
	  rect_tracker = rect_tracker_new(ji_screen, outx1, outy1, outx2, outy2);

	  dotted_mode(0);

	  /* draw the rectangle in the drawable region */
	  region = jwidget_get_drawable_region(widget, JI_GDR_CUTTOPWINDOWS);
	  nrects = JI_REGION_NUM_RECTS(region);
	  for (c=0, rc=JI_REGION_RECTS(region);
	       c<nrects;
	       c++, rc++) {
	    set_clip(ji_screen, rc->x1, rc->y1, rc->x2-1, rc->y2-1);
	    rect(ji_screen, outx1, outy1, outx2, outy2, 0);
	  }
	  set_clip(ji_screen, 0, 0, JI_SCREEN_W-1, JI_SCREEN_H-1);
	  jregion_free(region);

	  dotted_mode(-1);
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
	    outx2 = outx1 + sprite->w-1;
	    outy2 = outy1 + sprite->h-1;
	  }
	  else if (tool->flags & TOOL_UPDATE_POINT) {
	    outx1 = outx2 = x2;
	    outy1 = outy2 = y2;
	  }
	  else if (tool->flags & TOOL_UPDATE_TRACE) {
	    outx1 = MIN(x1, x2);
	    outy1 = MIN(y1, y2);
	    outx2 = MAX(x1, x2);
	    outy2 = MAX(y1, y2);
	  }
	  else if (tool->flags & TOOL_UPDATE_BOX) {
	    if (first_time) {
	      old_x1 = x1;
	      old_y1 = y1;
	      old_x2 = x2;
	      old_y2 = y2;
	    }

	    outx1 = MIN(MIN(x1, old_x1), MIN(x2, old_x2));
	    outy1 = MIN(MIN(y1, old_y1), MIN(y2, old_y2));
	    outx2 = MAX(MAX(x1, old_x1), MAX(x2, old_x2));
	    outy2 = MAX(MAX(y1, old_y1), MAX(y2, old_y2));

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
	  else if (tool->flags & TOOL_UPDATE_4PTS) {
	    if (tool->flags & TOOL_4FIRST2LAST) {
	      for (c=0; c<4; c++) {
		if (outx1 > old_pts[c*2  ]) outx1 = old_pts[c*2];
		if (outx2 < old_pts[c*2  ]) outx2 = old_pts[c*2];
		if (outy1 > old_pts[c*2+1]) outy1 = old_pts[c*2+1];
		if (outy2 < old_pts[c*2+1]) outy2 = old_pts[c*2+1];
	      }
	    }
	    for (c=0; c<4; c++) {
	      if (outx1 > pts[c*2  ]) outx1 = pts[c*2];
	      if (outx2 < pts[c*2  ]) outx2 = pts[c*2];
	      if (outy1 > pts[c*2+1]) outy1 = pts[c*2+1];
	      if (outy2 < pts[c*2+1]) outy2 = pts[c*2+1];
	    }
	  }

	  /* for non-tiled mode */
	  if (!tool_data.tiled) {
	    outx1 = MAX(outx1-brush->size/2-offset_x, 0);
	    outy1 = MAX(outy1-brush->size/2-offset_y, 0);
	    outx2 = MIN(outx2+brush->size/2-offset_x, sprite->w-1);
	    outy2 = MIN(outy2+brush->size/2-offset_y, sprite->h-1);
 
	    editors_draw_sprite(sprite, outx1, outy1, outx2, outy2);
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
			       sprite->w,
			       sprite->h, &x2, &y2);
	      rectfill(ji_screen, x1, y1, x2-1, y2-1, makecol(255, 0, 0));
	      vsync();
	    }
#endif

	    editors_draw_sprite_tiled(sprite, outx1, outy1, outx2, outy2);
	  }
	}
      }

      /* draw the cursor in the screen */
      editor_draw_cursor(widget, jmouse_x(0), jmouse_y(0));

      /* update the state-bar */
      if (jwidget_is_visible(statusbar)) {
	if (tool->flags & TOOL_UPDATE_BOX) {
	  char mode[256] = "";

	  if (current_tool == tools_list[TOOL_MARKER]) {
	    if (start_b & 1) {
	      if (key_shifts & KB_ALT_FLAG)
		strcat(mode, _("Replace"));
	      else
		strcat(mode, _("Union"));
	    }
	    else {
	      if (key_shifts & KB_ALT_FLAG)
		strcat(mode, _("Intersect"));
	      else
		strcat(mode, _("Subtract"));
	    }
	  }

	  statusbar_set_text(statusbar, 0,
			      "%s %3d %3d %s %3d %3d (%s %3d %3d) %s",
			      _start, x1, y1,
			      _end, x2, y2,
			      _size, ABS(x2-x1)+1, ABS(y2-y1)+1, mode);
	}
	else {
	  statusbar_set_text(statusbar, 0, "%s %3d %3d", _pos, x1, y1);
	}

	jwidget_flush_redraw(statusbar);
	jmanager_dispatch_messages(ji_get_default_manager());
      }

      release_bitmap(ji_screen);
      forced_update = FALSE;
      first_time = FALSE;

      /* draw extra stuff */
      editor_draw_grid_safe(widget);
      /* editor_draw_path_safe(widget, FALSE); */
    }

    /* draw mask */
    editor_draw_mask_safe(widget);

    /* spray updating process */
    if (current_tool == tools_list[TOOL_SPRAY]) {
      if (ji_clock-spray_time > (1000/20)*(100-air_speed)/100) {
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
    /* in curve we have to input four points */
    if ((tool->flags & TOOL_4FIRST2LAST) &&
    	(curve_pts < 3)) {
      ++curve_pts;
      click2 = MODE_CLICKANDCLICK;
      goto next_pts;
    }

    if (undo_is_enabled(sprite->undo))
      undo_set_label(sprite->undo, tool->name);

    /* marker *******************************************************/
    if (tool == tools_list[TOOL_MARKER]) {
      void (*func)(Mask *, int, int, int, int);

      if (start_b & 1)
	func = (key_shifts & KB_ALT_FLAG) ? mask_replace: mask_union;
      else
	func = (key_shifts & KB_ALT_FLAG) ? mask_intersect: mask_subtract;

      /* insert the undo operation */
      if (undo_is_enabled(sprite->undo)) {
	undo_set_mask(sprite->undo, sprite);
      }

      (*func)(sprite->mask,
	      MIN(x1, x2) - offset_x,
	      MIN(y1, y2) - offset_y,
	      MAX(x1, x2) - MIN(x1, x2) + 1,
	      MAX(y1, y2) - MIN(y1, y2) + 1);

      sprite_generate_mask_boundaries(sprite);
      update_screen_for_sprite(sprite);

      destroy_cel = TRUE;
    }

    /* draw trace ***************************************************/
    else {
      /* if the size of each image is the same, we can create an undo
	 with only the differences between both images */
      if (cel->x == old_cel_x &&
	  cel->y == old_cel_y &&
	  cel_image->w == tool_data.dst_image->w &&
	  cel_image->h == tool_data.dst_image->h) {
	/* was the 'cel_image' created in the start of the tool-loop? */
	if (cel_created) {
	  /* then we can keep the 'cel_image'... */
	  
	  /* we copy the 'destination' image to the 'cel_image' */
	  image_copy(cel_image, tool_data.dst_image, 0, 0);

	  /* add the 'cel_image' in the images' stock of the sprite */
	  cel->image = stock_add_image(sprite->stock, cel_image);

	  /* is the undo enabled? */
	  if (undo_is_enabled(sprite->undo)) {
	    /* we can temporary remove the cel */
	    layer_remove_cel(sprite->layer, cel);

	    /* we create the undo information (for the new cel_image
	       in the stock and the new cel in the layer)... */
	    undo_open(sprite->undo);
	    undo_add_image(sprite->undo, sprite->stock, cel->image);
	    undo_add_cel(sprite->undo, sprite->layer, cel);
	    undo_close(sprite->undo);

	    /* and finally we add the cel again in the layer */
	    layer_add_cel(sprite->layer, cel);
	  }
	}
	else {
	  /* undo the dirty region */
	  if (undo_is_enabled(sprite->undo)) {
	    Dirty *dirty = dirty_new_from_differences(cel_image,
						      tool_data.dst_image);
	    /* TODO error handling: if (dirty == NULL) */

	    dirty_save_image_data(dirty);
	    if (dirty != NULL)
	      undo_dirty(sprite->undo, dirty);

	    dirty_free(dirty);
	  }

	  /* copy the 'dst_image' to the cel_image */
	  image_copy(cel_image, tool_data.dst_image, 0, 0);
	}
      }
      /* if the size of both images are different, we have to replace
	 the entire image */
      else {
	if (undo_is_enabled(sprite->undo)) {
	  undo_open(sprite->undo);

	  if (cel->x != old_cel_x) {
	    x1 = cel->x;
	    cel->x = old_cel_x;
	    undo_int(sprite->undo, (GfxObj *)cel, &cel->x);
	    cel->x = x1;
	  }
	  if (cel->y != old_cel_y) {
	    y1 = cel->y;
	    cel->y = old_cel_y;
	    undo_int(sprite->undo, (GfxObj *)cel, &cel->y);
	    cel->y = y1;
	  }

	  undo_replace_image(sprite->undo, sprite->stock, cel->image);
	  undo_close(sprite->undo);
	}

	/* replace the image in the stock */
	stock_replace_image(sprite->stock, cel->image, tool_data.dst_image);

	/* destroy the old cel image */
	image_free(cel_image);

	/* now the `dst_image' is used, so we haven't to destroy it */
	tool_data.dst_image = NULL;
      }
    }
  }
  /* if the user canceled the operation we have to restore the cel
     position */
  else
    destroy_cel = TRUE;

  if (destroy_cel) {
    cel->x = old_cel_x;
    cel->y = old_cel_y;

    if (cel_created) {
      layer_remove_cel(sprite->layer, cel);
      cel_free(cel);
      image_free(cel_image);
    }
  }

  /* redraw all the sprites */
  update_screen_for_sprite(sprite);

  editor_click_done(widget);

  /* no more preview image */
  set_preview_image(NULL, NULL);

  /* destroy temporary images */
  if (tool_data.src_image) image_free(tool_data.src_image);
  if (tool_data.dst_image) image_free(tool_data.dst_image);

  /* destroy rect-data used by the marker tool */
  if (rect_tracker) {
    rect_tracker_free(rect_tracker);
    rect_tracker = NULL;
  }
}

void apply_grid(int *x, int *y, bool flexible)
{
  div_t d, dx, dy;
  int w = jrect_w(grid);
  int h = jrect_h(grid);

  flexible = flexible ? 1: 0;

  dx = div(grid->x1, w);
  dy = div(grid->y1, h);

  d = div((*x)-dx.rem, w);
  *x = dx.rem + d.quot*w + ((d.rem > w/2)? w-flexible: 0);

  d = div((*y)-dy.rem, h);
  *y = dy.rem + d.quot*h + ((d.rem > h/2)? h-flexible: 0);
}

static void line_for_spline(int x1, int y1, int x2, int y2, ToolData *data)
{
  algo_line(x1, y1, x2, y2, data,
	    (brush->size == 1) ?
	    (AlgoPixel)do_ink_pixel:
	    (AlgoPixel)do_ink_brush);
}

/**********************************************************************/
/* Ink Processing   						      */
/**********************************************************************/

#define DEFINE_INK_PROCESSING(addresses_define,				\
			      addresses_initialize,			\
			      addresses_increment,			\
			      processing)				\
  addresses_define;							\
  register int x;							\
									\
  /* with mask */							\
  if (data->mask != NULL) {						\
    int (*getpixel)(const Image *, int, int) =				\
      data->mask->bitmap->method->getpixel;				\
									\
    if ((y < data->mask_y) || (y >= data->mask_y+data->mask->h))	\
      return;								\
									\
    if (x1 < data->mask_x)						\
      x1 = data->mask_x;						\
									\
    if (x2 > data->mask_x+data->mask->w-1)				\
      x2 = data->mask_x+data->mask->w-1;				\
									\
    if (data->mask->bitmap != NULL) {					\
      addresses_initialize;						\
      for (x=x1; x<=x2; ++x) {						\
	if (getpixel(data->mask->bitmap,				\
		     x-data->mask_x,					\
		     y-data->mask_y))					\
	  processing;							\
									\
	addresses_increment;						\
      }									\
      return;								\
    }									\
  }									\
									\
  addresses_initialize;							\
  for (x=x1; x<=x2; ++x) {						\
    processing;								\
    addresses_increment;						\
  }

#define DEFINE_INK_PROCESSING_DST(type, processing)				  \
  DEFINE_INK_PROCESSING(register type *dst_address;				, \
			dst_address = ((type **)data->dst_image->line)[y]+x1;	, \
			++dst_address						, \
			processing)

#define DEFINE_INK_PROCESSING_SRCDST(type, processing)				  \
  DEFINE_INK_PROCESSING(register type *src_address;				  \
			register type *dst_address;				, \
			src_address = ((type **)data->src_image->line)[y]+x1;	  \
			dst_address = ((type **)data->dst_image->line)[y]+x1;	, \
			++src_address;						  \
			++dst_address;						, \
			processing)

/**********************************************************************/
/* Opaque Ink   						      */
/**********************************************************************/

static void ink_hline4_opaque(int x1, int y, int x2, ToolData *data)
{
  int c = data->color;

  DEFINE_INK_PROCESSING_DST
    (ase_uint32,
     *dst_address = c			);
}

static void ink_hline2_opaque(int x1, int y, int x2, ToolData *data)
{
  int c = data->color;

  DEFINE_INK_PROCESSING_DST
    (ase_uint16,
     *dst_address = c			);
}

static void ink_hline1_opaque(int x1, int y, int x2, ToolData *data)
{
  int c = data->color;

  DEFINE_INK_PROCESSING_DST
    (ase_uint8,
     *dst_address = c			);

  /* memset(((ase_uint8 **)data->dst_image->line)[y]+x1, data->color, x2-x1+1); */
}

/**********************************************************************/
/* Glass Ink     						      */
/**********************************************************************/

static void ink_hline4_glass(int x1, int y, int x2, ToolData *data)
{
  int color = data->color;
  int opacity = data->opacity;

  DEFINE_INK_PROCESSING_SRCDST
    (ase_uint32,
     *dst_address = _rgba_blend_normal(*src_address, color, opacity));
}  

static void ink_hline2_glass(int x1, int y, int x2, ToolData *data)
{
  int color = data->color;
  int opacity = data->opacity;

  DEFINE_INK_PROCESSING_SRCDST
    (ase_uint16,
     *dst_address = _graya_blend_normal(*src_address, color, opacity));
}  

static void ink_hline1_glass(int x1, int y, int x2, ToolData *data)
{
  Palette *pal = get_current_palette();
  ase_uint32 c;
  ase_uint32 tc = pal->color[data->color];
  int opacity = data->opacity;

  DEFINE_INK_PROCESSING_SRCDST
    (ase_uint8,
     {
       c = _rgba_blend_normal(pal->color[*src_address], tc, opacity);
       *dst_address = orig_rgb_map->data
	 [_rgba_getr(c)>>3]
	 [_rgba_getg(c)>>3]
	 [_rgba_getb(c)>>3];
     });
}  

/**********************************************************************/
/* Soften Ink     						      */
/**********************************************************************/

static void ink_hline4_soften(int x1, int y, int x2, ToolData *data)
{
  int c, r, g, b, a;
  int opacity = data->opacity;
  bool tiled = data->tiled;
  Image *src = data->src_image;
  int getx, gety;
  int addx, addy;
  int dx, dy, color;
  ase_uint32 *src_address2;

  DEFINE_INK_PROCESSING_SRCDST
    (ase_uint32,
     {
       c = 0;
       r = g = b = a = 0;

       GET_MATRIX_DATA
	 (ase_uint32, src, src_address2,
	  3, 3, 1, 1, tiled,
	  color = *src_address2;
	  if (_rgba_geta(color) != 0) {
	    r += _rgba_getr(color);
	    g += _rgba_getg(color);
	    b += _rgba_getb(color);
	    a += _rgba_geta(color);
	    ++c;
	  }
	  );

       if (c > 0) {
	 r /= c;
	 g /= c;
	 b /= c;
	 a /= 9;

	 c = *src_address;
	 r = _rgba_getr(c) + (r-_rgba_getr(c)) * opacity / 255;
	 g = _rgba_getg(c) + (g-_rgba_getg(c)) * opacity / 255;
	 b = _rgba_getb(c) + (b-_rgba_getb(c)) * opacity / 255;
	 a = _rgba_geta(c) + (a-_rgba_geta(c)) * opacity / 255;
	 
	 *dst_address = _rgba(r, g, b, a);
       }
       else {
	 *dst_address = *src_address;
       }
     });
}  

static void ink_hline2_soften(int x1, int y, int x2, ToolData *data)
{
  int c, v, a;
  int opacity = data->opacity;
  bool tiled = data->tiled;
  Image *src = data->src_image;
  int getx, gety;
  int addx, addy;
  int dx, dy, color;
  ase_uint16 *src_address2;

  DEFINE_INK_PROCESSING_SRCDST
    (ase_uint16,
     {
       c = 0;
       v = a = 0;

       GET_MATRIX_DATA
	 (ase_uint16, src, src_address2,
	  3, 3, 1, 1, tiled,
	  color = *src_address2;
	  if (_graya_geta(color) > 0) {
	    v += _graya_getv(color);
	    a += _graya_geta(color);
	  }
	  c++;
	  );

       if (c > 0) {
	 v /= c;
	 a /= 9;

	 c = *src_address;
	 v = _graya_getv(c) + (v-_graya_getv(c)) * opacity / 255;
	 a = _graya_geta(c) + (a-_graya_geta(c)) * opacity / 255;
	 
	 *dst_address = _graya(v, a);
       }
       else {
	 *dst_address = *src_address;
       }
     });
}  

static void ink_hline1_soften(int x1, int y, int x2, ToolData *data)
{
  Palette *pal = get_current_palette();
  int c, r, g, b, a;
  int opacity = data->opacity;
  bool tiled = data->tiled;
  Image *src = data->src_image;
  int getx, gety;
  int addx, addy;
  int dx, dy, color;
  ase_uint8 *src_address2;
  
  DEFINE_INK_PROCESSING_SRCDST
    (ase_uint8,
     {
       c = 0;
       r = g = b = a = 0;

       GET_MATRIX_DATA
	 (ase_uint8, src, src_address2,
	  3, 3, 1, 1, tiled,

	  color = *src_address2;
	  a += (color == 0 ? 0: 255);

	  color = pal->color[color];
	  r += _rgba_getr(color);
	  g += _rgba_getg(color);
	  b += _rgba_getb(color);
	  c++;
	  );

       if (c > 0 && a/9 >= 128) {
	 r /= c;
	 g /= c;
	 b /= c;

	 c = pal->color[*src_address];
	 r = _rgba_getr(c) + (r-_rgba_getr(c)) * opacity / 255;
	 g = _rgba_getg(c) + (g-_rgba_getg(c)) * opacity / 255;
	 b = _rgba_getb(c) + (b-_rgba_getb(c)) * opacity / 255;

	 *dst_address = orig_rgb_map->data[r>>3][g>>3][b>>3];
       }
       else {
	 *dst_address = *src_address;
       }
     });
}  

/**********************************************************************/
/* Replace Ink     						      */
/**********************************************************************/

static void ink_hline4_replace(int x1, int y, int x2, ToolData *data)
{
  int color = data->color;
  int other_color = data->other_color;
  int opacity = data->opacity;

  DEFINE_INK_PROCESSING_SRCDST
    (ase_uint32,
     if (*src_address == other_color) {
       *dst_address = _rgba_blend_normal(*src_address, color, opacity);
     });
}  

static void ink_hline2_replace(int x1, int y, int x2, ToolData *data)
{
  int color = data->color;
  int other_color = data->other_color;
  int opacity = data->opacity;

  DEFINE_INK_PROCESSING_SRCDST
    (ase_uint16,
     if (*src_address == other_color) {
       *dst_address = _graya_blend_normal(*src_address, color, opacity);
     });
}  

static void ink_hline1_replace(int x1, int y, int x2, ToolData *data)
{
  int other_color = data->other_color;
  Palette *pal = get_current_palette();
  ase_uint32 c;
  ase_uint32 tc = pal->color[data->color];
  int opacity = data->opacity;

  DEFINE_INK_PROCESSING_SRCDST
    (ase_uint8,
     if (*src_address == other_color) {
       c = _rgba_blend_normal(pal->color[*src_address], tc, opacity);
       *dst_address = orig_rgb_map->data
	 [_rgba_getr(c)>>3]
	 [_rgba_getg(c)>>3]
	 [_rgba_getb(c)>>3];
     });
}

/**********************************************************************/
/* Jumble Ink     						      */
/**********************************************************************/

#define JUMBLE_XY_IN_UV()						\
  u = x + (rand() % 3)-1 - speed_x;					\
  v = y + (rand() % 3)-1 - speed_y;					\
  									\
  if (tiled) {								\
    if (u < 0)								\
      u = data->src_image->w - (-(u+1) % data->src_image->w) - 1;	\
    else if (u >= data->src_image->w)					\
      u %= data->src_image->w;						\
									\
    if (v < 0)								\
      v = data->src_image->h - (-(v+1) % data->src_image->h) - 1;	\
    else if (v >= data->src_image->h)					\
      v %= data->src_image->h;						\
  }									\
  else {								\
    u = MID(0, u, data->src_image->w-1);				\
    v = MID(0, v, data->src_image->h-1);				\
  }									\
  color = image_getpixel(data->src_image, u, v);

static void ink_hline4_jumble(int x1, int y, int x2, ToolData *data)
{
  int opacity = data->opacity;
  int speed_x = data->vector.x/4;
  int speed_y = data->vector.y/4;
  bool tiled = data->tiled;
  int u, v, color;

  DEFINE_INK_PROCESSING_SRCDST
    (ase_uint32,
     {
       JUMBLE_XY_IN_UV();
       *dst_address = _rgba_blend_MERGE(*src_address, color, opacity);
     }
     );
}  

static void ink_hline2_jumble(int x1, int y, int x2, ToolData *data)
{
  int opacity = data->opacity;
  int speed_x = data->vector.x/4;
  int speed_y = data->vector.y/4;
  bool tiled = data->tiled;
  int u, v, color;

  DEFINE_INK_PROCESSING_SRCDST
    (ase_uint16,
     {
       JUMBLE_XY_IN_UV();
       *dst_address = _graya_blend_MERGE(*src_address, color, opacity);
     }
     );
}  

static void ink_hline1_jumble(int x1, int y, int x2, ToolData *data)
{
  Palette *pal = get_current_palette();
  ase_uint32 c, tc;
  int opacity = data->opacity;
  int speed_x = data->vector.x/4;
  int speed_y = data->vector.y/4;
  bool tiled = data->tiled;
  int u, v, color;

  DEFINE_INK_PROCESSING_SRCDST
    (ase_uint8,
     {
       JUMBLE_XY_IN_UV();

       tc = color != 0 ? pal->color[color]: 0;
       c = _rgba_blend_MERGE(*src_address != 0 ? pal->color[*src_address]: 0,
			     tc, opacity);

       if (_rgba_geta(c) >= 128)
	 *dst_address = orig_rgb_map->data
	   [_rgba_getr(c)>>3]
	   [_rgba_getg(c)>>3]
	   [_rgba_getb(c)>>3];
       else
	 *dst_address = 0;
     }
     );
}  
