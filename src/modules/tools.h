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

#ifndef MODULES_TOOLS_H
#define MODULES_TOOLS_H

#include "jinete/jbase.h"
#include "jinete/jrect.h"

struct _GList;
struct Brush;
struct Dirty;
struct Sprite;

#define TOOL_ACCUMULATE_DIRTY   (1<<1)
#define TOOL_FIRST2LAST         (1<<2)
#define TOOL_OLD2LAST           (1<<3)
#define TOOL_FOURCHAIN          (1<<4)
#define TOOL_ACCEPT_FILL        (1<<5)
#define TOOL_UPDATE_ALL         (1<<6)
#define TOOL_UPDATE_POINT       (1<<7)
#define TOOL_UPDATE_TRACE       (1<<8)
#define TOOL_UPDATE_BOX         (1<<9)
#define TOOL_UPDATE_SPRAY       (1<<10)
#define TOOL_UPDATE_LAST4       (1<<11)
#define TOOL_EIGHT_ANGLES	(1<<12)

enum {
  DRAWMODE_OPAQUE,
  DRAWMODE_GLASS,
  DRAWMODE_SEMI,
};

typedef struct Tool Tool;

struct Tool
{
  const char *name;
  const char *translated_name;
  int flags;
  void (*put) (struct Dirty *dirty, int x1, int y1, int x2, int y2);
};

extern Tool *current_tool;

extern Tool ase_tool_marker;
extern Tool ase_tool_dots;
extern Tool ase_tool_pencil;
extern Tool ase_tool_brush;
extern Tool ase_tool_floodfill;
extern Tool ase_tool_spray;
extern Tool ase_tool_line;
extern Tool ase_tool_rectangle;
extern Tool ase_tool_ellipse;

extern Tool *ase_tools_list[];

int init_module_tools(void);
void exit_module_tools(void);

void refresh_tools_names(void);
void select_tool(Tool *tool);
void select_tool_by_name(const char *tool_name);

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
const char *get_cursor_color(void);
void set_cursor_color(const char *color);

int get_thickness_for_cursor(void);

void control_tool(JWidget editor, Tool *tool, const char *color);

void do_tool_points(struct Sprite *sprite, Tool *tool, const char *color,
		    int npoints, int *x, int *y);

void apply_grid(int *x, int *y);

#endif /* MODULES_TOOLS_H */

