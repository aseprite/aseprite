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

#ifndef USE_PRECOMPILED_HEADER

#include <stdlib.h>
#include <string.h>

#include "core/app.h"
#include "core/cfg.h"
#include "modules/color.h"
#include "modules/sprites.h"
#include "modules/tools.h"
#include "raster/brush.h"
#include "raster/sprite.h"

#endif

/* string = "brush_type brush_size brush_angle"
     brush_type: circle, square, line (default=circle)
     brush_size: 1-32 (default=1)
     brush_angle: 0-180 (default=0)
 */
void SetBrush(const char *string)
{
  int type = BRUSH_CIRCLE;
  int size = 1;
  int angle = 0;
  char *copy = jstrdup(string);
  char *tok;
  int count;

  for (tok=strtok(copy, " "), count=0; tok;
       tok=strtok(NULL, " "), count++) {
    switch (count) {

      case 0:
	if (strcmp(tok, "circle") == 0)
	  type = BRUSH_CIRCLE;
	else if (strcmp(tok, "square") == 0)
	  type = BRUSH_SQUARE;
	else if (strcmp(tok, "line") == 0)
	  type = BRUSH_LINE;
	break;

      case 1:
	size = strtol(tok, NULL, 0);
	break;

      case 2:
	angle = strtol(tok, NULL, 0);
	break;
    }
  }

  set_brush_type(type);
  set_brush_size(MID(1, size, 32));
  set_brush_angle(MID(0, angle, 180));

  jfree(copy);
}

/* string = "draw_mode glass_dirty" 
     draw_mode: opaque, glass, semi (default=opaque)
     glass_dirty: 0-255 (default=128)
 */
void SetDrawMode(const char *string)
{
  char *copy = jstrdup(string);
  char *tok;
  int count;
  int draw_mode = DRAWMODE_OPAQUE;
  int glass_dirty = 128;

  for (tok=strtok(copy, " "), count=0; tok;
       tok=strtok(NULL, " "), count++) {
    switch (count) {

      case 0:
	if (strcmp(tok, "opaque") == 0)
	  draw_mode = DRAWMODE_OPAQUE;
	else if (strcmp(tok, "glass") == 0)
	  draw_mode = DRAWMODE_GLASS;
	else if (strcmp(tok, "semi") == 0)
	  draw_mode = DRAWMODE_SEMI;
	break;

      case 1:
	glass_dirty = strtol(tok, NULL, 0);
	break;
    }
  }

  set_brush_mode(draw_mode);
  set_glass_dirty(MID(0, glass_dirty, 255));

  jfree(copy);
}

/* string = "tool_name x,y x,y x,y ..." 
     tool_name: marker, dots, pencil, brush, floodfill, spray, line,
                rectangle, ellipse
     uses the current FG color
 */
void ToolTrace(const char *string, const char *color)
{
  Sprite *sprite = current_sprite;

  if (sprite) {
    Tool *old_current_tool = current_tool;
    char *copy = jstrdup(string);
    char *tok;
    int count;
    int npoints = 0;
    int *x = NULL;
    int *y = NULL;

    for (tok=strtok(copy, " "), count=0; tok;
	 tok=strtok(NULL, " "), count++) {
      switch (count) {
	case 0:
	  select_tool(tok);
	  break;
	default: {
	  int u, v;
	  char *s;

	  u = (int)strtod(tok, &s);
	  if (s && *s == ',') {
	    v = (int)strtod(s+1, &s);

	    npoints++;
	    x = jrealloc(x, sizeof(int)*npoints);
	    y = jrealloc(y, sizeof(int)*npoints);
	    x[npoints-1] = u;
	    y[npoints-1] = v;
	  }
	  break;
	}
      }
    }

    if (npoints > 0) {
      do_tool_points(sprite, current_tool, color, npoints, x, y);
      jfree(x);
      jfree(y);
    }

    select_tool(old_current_tool->name);

    jfree(copy);
  }
}

/**********************************************************************/
/* Reset/Restore Configuration */

static bool cfg_options_move_mask;
static bool cfg_target_red;
static bool cfg_target_green;
static bool cfg_target_blue;
static bool cfg_target_gray;
static bool cfg_target_alpha;
static bool cfg_target_index;
static int cfg_target_images;
static int cfg_tools_brush_type;
static int cfg_tools_brush_size;
static int cfg_tools_brush_angle;
static int cfg_tools_brush_mode;
static int cfg_tools_glass_dirty;
static int cfg_tools_spray_width;
static int cfg_tools_air_speed;
static bool cfg_tools_filled;
static bool cfg_tools_tiled;
static bool cfg_tools_grid_use;
static bool cfg_tools_grid_view;
static JRect cfg_tools_grid_rect = NULL;
static bool cfg_tools_onionskin;
static int cfg_jpeg_smooth;
static int cfg_jpeg_quality;
static int cfg_jpeg_method;

void ResetConfig(void)
{
  JRect rect;

  /* movement */
  cfg_options_move_mask = get_config_bool("QuickMovement", "UseMask", TRUE);

  set_config_bool("QuickMovement", "UseMask", TRUE);

  /* targets */
  cfg_target_red = get_config_bool("Target", "Red", TRUE);
  cfg_target_green = get_config_bool("Target", "Green", TRUE);
  cfg_target_blue = get_config_bool("Target", "Blue", TRUE);
  cfg_target_gray = get_config_bool("Target", "Gray", TRUE);
  cfg_target_alpha = get_config_bool("Target", "Alpha", TRUE);
  cfg_target_index = get_config_bool("Target", "Index", TRUE);
  cfg_target_images = get_config_int("Target", "Images", 0);

  set_config_bool("Target", "Red", TRUE);
  set_config_bool("Target", "Green", TRUE);
  set_config_bool("Target", "Blue", TRUE);
  set_config_bool("Target", "Gray", TRUE);
  set_config_bool("Target", "Alpha", TRUE);
  set_config_bool("Target", "Index", TRUE);
  set_config_int("Target", "Images", 0);

  /* brush */
  cfg_tools_brush_type = get_brush_type();
  cfg_tools_brush_size = get_brush_size();
  cfg_tools_brush_angle = get_brush_angle();
  cfg_tools_brush_mode = get_brush_mode();
  cfg_tools_glass_dirty = get_glass_dirty();
  cfg_tools_spray_width = get_spray_width();
  cfg_tools_air_speed = get_air_speed();
  cfg_tools_filled = get_filled_mode();
  cfg_tools_tiled = get_tiled_mode();
  cfg_tools_grid_use = get_use_grid();
  cfg_tools_grid_view = get_view_grid();

  if (cfg_tools_grid_rect)
    jrect_free(cfg_tools_grid_rect);
  cfg_tools_grid_rect = get_grid();

  cfg_tools_onionskin = get_onionskin();

  set_brush_type(BRUSH_CIRCLE);
  set_brush_size(1);
  set_brush_angle(0);
  set_brush_mode(DRAWMODE_OPAQUE);
  set_cursor_color("mask");
  set_glass_dirty(128);
  set_spray_width(16);
  set_air_speed(75);
  set_filled_mode(FALSE);
  set_tiled_mode(FALSE);
  set_use_grid(FALSE);
  set_view_grid(FALSE);
  rect = jrect_new(0, 0, 16, 16);
  set_grid(rect);
  jrect_free(rect);
  set_onionskin(FALSE);

  /* jpeg options */
  cfg_jpeg_smooth = get_config_int("JPEG", "Smooth", 0);
  cfg_jpeg_quality = get_config_int("JPEG", "Quality", 100);
  cfg_jpeg_method = get_config_int("JPEG", "Method", 0);

  set_config_int("JPEG", "Smooth", 0);
  set_config_int("JPEG", "Quality", 100);
  set_config_int("JPEG", "Method", 0);
}

void RestoreConfig(void)
{
  /* movement */
  set_config_bool("QuickMovement", "UseMask", cfg_options_move_mask);

  /* targets */
  set_config_bool("Target", "Red", cfg_target_red);
  set_config_bool("Target", "Green", cfg_target_green);
  set_config_bool("Target", "Blue", cfg_target_blue);
  set_config_bool("Target", "Gray", cfg_target_gray);
  set_config_bool("Target", "Alpha", cfg_target_alpha);
  set_config_bool("Target", "Index", cfg_target_index);
  set_config_int("Target", "Images", cfg_target_images);

  /* brush */
  set_brush_type(cfg_tools_brush_type);
  set_brush_size(cfg_tools_brush_size);
  set_brush_angle(cfg_tools_brush_angle);
  set_brush_mode(cfg_tools_brush_mode);
  set_glass_dirty(cfg_tools_glass_dirty);
  set_spray_width(cfg_tools_spray_width);
  set_air_speed(cfg_tools_air_speed);
  set_filled_mode(cfg_tools_filled);
  set_tiled_mode(cfg_tools_tiled);
  set_use_grid(cfg_tools_grid_use);
  set_view_grid(cfg_tools_grid_view);

  set_grid(cfg_tools_grid_rect);
  jrect_free(cfg_tools_grid_rect);
  cfg_tools_grid_rect = NULL;

  set_onionskin(cfg_tools_onionskin);

  /* jpeg options */
  set_config_int("JPEG", "Smooth", cfg_jpeg_smooth);
  set_config_int("JPEG", "Quality", cfg_jpeg_quality);
  set_config_int("JPEG", "Method", cfg_jpeg_method);
}
