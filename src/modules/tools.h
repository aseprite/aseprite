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

#ifndef MODULES_TOOLS_H
#define MODULES_TOOLS_H

#include "jinete/jbase.h"
#include "jinete/jrect.h"

#include "core/color.h"
#include "raster/algo.h"

struct Brush;
struct Image;
struct Layer;
struct Mask;
struct Sprite;

enum {
  TOOL_MARKER,
  TOOL_PENCIL,
  TOOL_BRUSH,
  TOOL_ERASER,
  TOOL_FLOODFILL,
  TOOL_SPRAY,
  TOOL_LINE,
  TOOL_RECTANGLE,
  TOOL_ELLIPSE,
  TOOL_BLUR,
  TOOL_GUMBLE,
  MAX_TOOLS
};

typedef struct Tool Tool;
typedef struct ToolData ToolData;

struct Tool
{
  const char *key;
  const char *name;
  const char *tips;
  int flags;
  void (*preprocess_data)(ToolData *data);
  void (*draw_trace)(int x1, int y1, int x2, int y2, ToolData *data);
  JAccel accel;
};

struct ToolData
{
  struct Sprite *sprite;
  struct Layer *layer;
  struct Image *src_image;	/* where we can get pixels (readonly image) */
  struct Image *dst_image;	/* where we should put pixels */
  struct Brush *brush;		/* brush to be used with the ink */
  struct Mask *mask;		/* mask to limit the paint */
  int mask_x, mask_y;		/* mask offset */
  int color;			/* primary color to draw */
  int other_color;		/* secondary color to draw */
  bool left_button;		/* did the user start the trace with the left mouse button? */
  AlgoHLine ink_hline_proc;
  int opacity;
  bool tiled;
  struct {
    int x;
    int y;
  } vector;			/* vector of direction */
};

extern Tool *current_tool;
extern Tool *tools_list[];

int init_module_tools(void);
void exit_module_tools(void);

void tool_add_key(Tool *tool, const char *string);
bool tool_is_key_pressed(Tool *tool, JMessage msg);

Tool *get_tool_by_name(const char *name);
Tool *get_tool_by_key(JMessage msg);

void select_tool(Tool *tool);

struct Brush *get_brush(void);
int get_brush_type(void);
int get_brush_size(void);
int get_brush_angle(void);
int get_brush_mode(void);
int get_glass_dirty(void);
int get_spray_width(void);
int get_air_speed(void);
bool get_filled_mode(void);
bool get_tiled_mode(void);
bool get_use_grid(void);
bool get_view_grid(void);
JRect get_grid(void);
bool get_onionskin(void);

void set_brush_type(int type);
void set_brush_size(int size);
void set_brush_angle(int angle);
void set_brush_mode(int mode);
void set_glass_dirty(int glass_dirty);
void set_spray_width(int spray_width);
void set_air_speed(int air_speed);
void set_filled_mode(bool status);
void set_tiled_mode(bool status);
void set_use_grid(bool status);
void set_view_grid(bool status);
void set_grid(JRect rect);
void set_onionskin(bool status);

int get_raw_cursor_color(void);
bool is_cursor_mask(void);
color_t get_cursor_color(void);
void set_cursor_color(color_t color);

int get_thickness_for_cursor(void);

void control_tool(JWidget editor, Tool *tool,
		  color_t color,
		  color_t other_color,
		  bool left_button);

/* void do_tool_points(struct Sprite *sprite, Tool *tool, color_t color, */
/* 		    int npoints, int *x, int *y); */

void apply_grid(int *x, int *y, bool flexible);

#endif /* MODULES_TOOLS_H */

