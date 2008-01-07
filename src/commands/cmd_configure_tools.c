/* ASE - Allegro Sprite Editor
 * Copyright (C) 2007, 2008  David A. Capello
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

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "core/app.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/rootmenu.h"
#include "modules/sprites.h"
#include "modules/tools.h"
#include "raster/brush.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "widgets/colbut.h"
#include "widgets/editor.h"
#include "widgets/groupbut.h"
#include "widgets/statebar.h"

#endif

static JWidget window = NULL;

static bool brush_preview_msg_proc(JWidget widget, JMessage msg);

static int window_close_hook(JWidget widget, int user_data);
static int brush_size_slider_change_hook(JWidget widget, int user_data);
static int brush_angle_slider_change_hook(JWidget widget, int user_data);
static int brush_type_change_hook(JWidget widget, int user_data);
static int brush_mode_change_hook(JWidget widget, int user_data);
static int glass_dirty_slider_change_hook(JWidget widget, int user_data);
static int spray_width_slider_change_hook(JWidget widget, int user_data);
static int air_speed_slider_change_hook(JWidget widget, int user_data);
static int filled_check_change_hook(JWidget widget, int user_data);
static int tiled_check_change_hook(JWidget widget, int user_data);
static int use_grid_check_change_hook(JWidget widget, int user_data);
static int view_grid_check_change_hook(JWidget widget, int user_data);
static int set_grid_button_select_hook(JWidget widget, int user_data);
static int cursor_button_change_hook(JWidget widget, int user_data);
static int onionskin_check_change_hook(JWidget widget, int user_data);

static void cmd_configure_tools_execute(const char *argument)
{
  JWidget filled, tiled, use_grid, view_grid, set_grid;
  JWidget brush_size, brush_angle, glass_dirty;
  JWidget spray_width, air_speed;
  JWidget cursor_color, cursor_color_box;
  JWidget brush_preview_box;
  JWidget brush_type_box, brush_type;
  JWidget brush_mode_box, brush_mode;
  JWidget check_onionskin;
  JWidget brush_preview;

  /* if the window is opened, close it */
  if (window) {
    jwindow_close(window, NULL);
    return;
  }

  /* if the window is closed, open it */

  window = load_widget("toolconf.jid", "configure_tool");
  if (!window)
    return;

  if (!get_widgets (window,
		    "filled", &filled,
		    "tiled", &tiled,
		    "use_grid", &use_grid,
		    "view_grid", &view_grid,
		    "set_grid", &set_grid,
		    "brush_size", &brush_size,
		    "brush_angle", &brush_angle,
		    "glass_dirty", &glass_dirty,
		    "spray_width", &spray_width,
		    "air_speed", &air_speed,
		    "cursor_color_box", &cursor_color_box,
		    "brush_preview_box", &brush_preview_box,
		    "brush_type_box", &brush_type_box,
		    "brush_mode_box", &brush_mode_box,
		    "onionskin", &check_onionskin, NULL)) {
    jwidget_free(window);
    return;
  }

  /* cursor-color */
  cursor_color = color_button_new(get_cursor_color(), IMAGE_INDEXED);
  /* brush-preview */
  brush_preview = jwidget_new(JI_WIDGET);
  brush_preview->min_w = 32 + 4;
  brush_preview->min_h = 32 + 4;
  jwidget_add_hook(brush_preview, JI_WIDGET,
		   brush_preview_msg_proc, NULL);
  /* brush-type */
  brush_type = group_button_new(3, 1, get_brush_type(),
				GFX_BRUSH_CIRCLE,
				GFX_BRUSH_SQUARE,
				GFX_BRUSH_LINE);
  /* brush-type */
  brush_mode = group_button_new(3, 1, get_brush_mode(),
				GFX_DRAWMODE_OPAQUE,
				GFX_DRAWMODE_GLASS,
				GFX_DRAWMODE_SEMI);

  /* append children */
  jwidget_add_child(cursor_color_box, cursor_color);
  jwidget_add_child(brush_preview_box, brush_preview);
  jwidget_add_child(brush_type_box, brush_type);
  jwidget_add_child(brush_mode_box, brush_mode);

  if (get_filled_mode()) jwidget_select(filled);
  if (get_tiled_mode()) jwidget_select(tiled);
  if (get_use_grid()) jwidget_select(use_grid);
  if (get_view_grid()) jwidget_select(view_grid);
  jslider_set_value(brush_size, get_brush_size());
  jslider_set_value(brush_angle, get_brush_angle());
  jslider_set_value(glass_dirty, get_glass_dirty());
  jslider_set_value(spray_width, get_spray_width());
  jslider_set_value(air_speed, get_air_speed());
  if (get_onionskin()) jwidget_select(check_onionskin);

  HOOK(window, JI_SIGNAL_WINDOW_CLOSE, window_close_hook, 0);
  HOOK(filled, JI_SIGNAL_CHECK_CHANGE, filled_check_change_hook, 0);
  HOOK(tiled, JI_SIGNAL_CHECK_CHANGE, tiled_check_change_hook, 0);
  HOOK(use_grid, JI_SIGNAL_CHECK_CHANGE, use_grid_check_change_hook, 0);
  HOOK(view_grid, JI_SIGNAL_CHECK_CHANGE, view_grid_check_change_hook, 0);
  HOOK(set_grid, JI_SIGNAL_BUTTON_SELECT, set_grid_button_select_hook, 0);
  HOOK(brush_size, JI_SIGNAL_SLIDER_CHANGE, brush_size_slider_change_hook, brush_preview);
  HOOK(brush_angle, JI_SIGNAL_SLIDER_CHANGE, brush_angle_slider_change_hook, brush_preview);
  HOOK(brush_type, SIGNAL_GROUP_BUTTON_CHANGE, brush_type_change_hook, brush_preview);
  HOOK(brush_mode, SIGNAL_GROUP_BUTTON_CHANGE, brush_mode_change_hook, 0);
  HOOK(glass_dirty, JI_SIGNAL_SLIDER_CHANGE, glass_dirty_slider_change_hook, 0);
  HOOK(air_speed, JI_SIGNAL_SLIDER_CHANGE, air_speed_slider_change_hook, 0);
  HOOK(spray_width, JI_SIGNAL_SLIDER_CHANGE, spray_width_slider_change_hook, 0);
  HOOK(cursor_color, SIGNAL_COLOR_BUTTON_CHANGE, cursor_button_change_hook, 0);
  HOOK(check_onionskin, JI_SIGNAL_CHECK_CHANGE, onionskin_check_change_hook, 0);

  /* default position */
  jwindow_remap(window);
  jwindow_center(window);

  /* load window configuration */
  load_window_pos(window, "ConfigureTool");

  /* open the window */
  jwindow_open_bg(window);
}

static bool brush_preview_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DRAW: {
      BITMAP *bmp = create_bitmap(jrect_w(widget->rc),
				  jrect_h(widget->rc));
      Brush *brush = get_brush();

      clear_to_color(bmp, makecol(0, 0, 0));
      image_to_allegro(brush->image, bmp,
		       bmp->w/2 - brush->size/2,
		       bmp->h/2 - brush->size/2);
      blit(bmp, ji_screen, 0, 0, widget->rc->x1, widget->rc->y1,
	   bmp->w, bmp->h);
      destroy_bitmap(bmp);
      return TRUE;
    }
  }

  return FALSE;
}

static int window_close_hook(JWidget widget, int user_data)
{
  /* isn't running anymore */
  window = NULL;

  /* save window configuration */
  save_window_pos(widget, "ConfigureTool");

  return FALSE;
}

static int brush_size_slider_change_hook(JWidget widget, int user_data)
{
  set_brush_size(jslider_get_value(widget));
  jwidget_dirty((JWidget)user_data);
  return FALSE;
}

static int brush_angle_slider_change_hook(JWidget widget, int user_data)
{
  set_brush_angle(jslider_get_value(widget));
  jwidget_dirty((JWidget)user_data);
  return FALSE;
}

static int brush_type_change_hook(JWidget widget, int user_data)
{
  int type = group_button_get_selected(widget);

  set_brush_type(type);
  jwidget_dirty((JWidget)user_data);

  status_bar_set_text(app_get_status_bar(), 250,
		      "Brush type: %s",
		      type == BRUSH_CIRCLE ? "Circle":
		      type == BRUSH_SQUARE ? "Square":
		      type == BRUSH_LINE ? "Line": "Unknown");

  return TRUE;
}

static int brush_mode_change_hook(JWidget widget, int user_data)
{
  int mode = group_button_get_selected(widget);

  set_brush_mode(mode);

  status_bar_set_text(app_get_status_bar(), 250,
		      "Brush mode: %s",
		      mode == DRAWMODE_OPAQUE ? "Opaque":
		      mode == DRAWMODE_GLASS ? "Glass":
		      mode == DRAWMODE_SEMI ? "Semi": "Unknown");
  return TRUE;
}

static int glass_dirty_slider_change_hook(JWidget widget, int user_data)
{
  set_glass_dirty(jslider_get_value(widget));
  return FALSE;
}

static int spray_width_slider_change_hook(JWidget widget, int user_data)
{
  set_spray_width(jslider_get_value(widget));
  return FALSE;
}

static int air_speed_slider_change_hook(JWidget widget, int user_data)
{
  set_air_speed(jslider_get_value(widget));
  return FALSE;
}

static int filled_check_change_hook(JWidget widget, int user_data)
{
  set_filled_mode(jwidget_is_selected(widget));
  return FALSE;
}

static int tiled_check_change_hook(JWidget widget, int user_data)
{
  set_tiled_mode(jwidget_is_selected(widget));
  return FALSE;
}

static int use_grid_check_change_hook(JWidget widget, int user_data)
{
  set_use_grid(jwidget_is_selected(widget));
  return FALSE;
}

static int view_grid_check_change_hook(JWidget widget, int user_data)
{
  set_view_grid(jwidget_is_selected(widget));
  refresh_all_editors();
  return FALSE;
}

static int set_grid_button_select_hook(JWidget widget, int user_data)
{
  Sprite *sprite = current_sprite;

  if (sprite && sprite->mask && sprite->mask->bitmap) {
    JRect rect = jrect_new(sprite->mask->x,
			   sprite->mask->y,
			   sprite->mask->x+sprite->mask->w,
			   sprite->mask->y+sprite->mask->h);
    set_grid(rect);
    jrect_free(rect);

    if (get_view_grid())
      refresh_all_editors();
  }
  else {
    jalert(_("Error"
	     "<<You must select a sprite with mask."
	     "<<The boundaries of the mask will be used"
	     "<<to specify the grid area.||&OK"));
  }

  return TRUE;
}

static int cursor_button_change_hook(JWidget widget, int user_data)
{
  set_cursor_color(color_button_get_color(widget));
  return TRUE;
}

static int onionskin_check_change_hook(JWidget widget, int user_data)
{
  set_onionskin(jwidget_is_selected(widget));
  refresh_all_editors();
  return FALSE;
}


Command cmd_configure_tools = {
  CMD_CONFIGURE_TOOLS,
  NULL,
  NULL,
  cmd_configure_tools_execute,
  NULL
};
