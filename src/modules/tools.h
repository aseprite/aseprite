/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#ifndef MODULES_TOOLS_H_INCLUDED
#define MODULES_TOOLS_H_INCLUDED

#include "jinete/jbase.h"
#include "jinete/jrect.h"

#include "core/color.h"
#include "raster/algo.h"

class Image;
class Layer;
class Mask;
class Sprite;

struct Brush;

enum tiled_t {
  TILED_NONE    = 0,
  TILED_X_AXIS  = 1,
  TILED_Y_AXIS  = 2,
  TILED_BOTH    = 3,
};

enum {
  TOOL_MARKER,
  TOOL_PENCIL,
  TOOL_BRUSH,
  TOOL_ERASER,
  TOOL_FLOODFILL,
  TOOL_SPRAY,
  TOOL_LINE,
  TOOL_CURVE,
  TOOL_RECTANGLE,
  TOOL_ELLIPSE,
  TOOL_BLUR,
  TOOL_JUMBLE,
  MAX_TOOLS
};

struct ToolData;

struct Tool
{
  const char* key;
  const char* name;
  const char* tips;
  int flags;
  void (*preprocess_data)(ToolData *data);
  void (*draw_trace)(int x1, int y1, int x2, int y2, ToolData *data);
};

struct ToolData
{
  Sprite* sprite;
  Layer* layer;
  Image* src_image;		/* where we can get pixels (readonly image) */
  Image* dst_image;		/* where we should put pixels */
  Brush* brush;			/* brush to be used with the ink */
  Mask* mask;			/* mask to limit the paint */
  int mask_x, mask_y;		/* mask offset */
  int color;			/* primary color to draw */
  int other_color;		/* secondary color to draw */
  bool left_button;		/* did the user start the trace with the left mouse button? */
  AlgoHLine ink_hline_proc;
  int opacity;
  tiled_t tiled;
  struct {
    int x;
    int y;
  } vector;			/* vector of direction */
};

extern Tool *current_tool;
extern Tool *tools_list[MAX_TOOLS];

int init_module_tools();
void exit_module_tools();

Tool *get_tool_by_name(const char *name);

void select_tool(Tool *tool);

Brush* get_brush();
int get_brush_type();
int get_brush_size();
int get_brush_angle();
int get_brush_mode();
int get_glass_dirty();
int get_spray_width();
int get_air_speed();
bool get_filled_mode();
tiled_t get_tiled_mode();
bool get_use_grid();
bool get_view_grid();
JRect get_grid();
bool get_onionskin();

void set_brush_type(int type);
void set_brush_size(int size);
void set_brush_angle(int angle);
void set_brush_mode(int mode);
void set_glass_dirty(int glass_dirty);
void set_spray_width(int spray_width);
void set_air_speed(int air_speed);
void set_filled_mode(bool status);
void set_tiled_mode(tiled_t mode);
void set_use_grid(bool status);
void set_view_grid(bool status);
void set_grid(JRect rect);
void set_onionskin(bool status);

int get_raw_cursor_color();
bool is_cursor_mask();
color_t get_cursor_color();
void set_cursor_color(color_t color);

int get_thickness_for_cursor();

void control_tool(JWidget editor, Tool *tool,
		  color_t color,
		  color_t other_color,
		  bool left_button);

/* void do_tool_points(Sprite* sprite, Tool* tool, color_t color, */
/* 		    int npoints, int *x, int *y); */

void apply_grid(int *x, int *y, bool flexible);

#endif

