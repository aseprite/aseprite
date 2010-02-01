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

#include <allegro.h>

#include "jinete/jinete.h"

#include "commands/command.h"
#include "app.h"
#include "ui_context.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/rootmenu.h"
#include "modules/tools.h"
#include "raster/brush.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "widgets/colbut.h"
#include "widgets/editor.h"
#include "widgets/groupbut.h"
#include "widgets/statebar.h"
#include "sprite_wrappers.h"

class FreeWidget : public IAppHook
{
  JWidget m_widget;
public:
  FreeWidget(JWidget widget) : m_widget(widget) { }
  void on_event()
  {
    jwidget_free(m_widget);
  }
};

static Frame* window = NULL;

static bool brush_preview_msg_proc(JWidget widget, JMessage msg);

static bool window_close_hook(JWidget widget, void *data);
static bool brush_size_slider_change_hook(JWidget widget, void *data);
static bool brush_angle_slider_change_hook(JWidget widget, void *data);
static bool brush_type_change_hook(JWidget widget, void *data);
static bool glass_dirty_slider_change_hook(JWidget widget, void *data);
static bool spray_width_slider_change_hook(JWidget widget, void *data);
static bool air_speed_slider_change_hook(JWidget widget, void *data);
static bool filled_check_change_hook(JWidget widget, void *data);
static bool tiled_check_change_hook(JWidget widget, void *data);
static bool tiled_xy_check_change_hook(JWidget widget, void *data);
static bool use_grid_check_change_hook(JWidget widget, void *data);
static bool view_grid_check_change_hook(JWidget widget, void *data);
static bool set_grid_button_select_hook(JWidget widget, void *data);
static bool cursor_button_change_hook(JWidget widget, void *data);
static bool onionskin_check_change_hook(JWidget widget, void *data);

//////////////////////////////////////////////////////////////////////

class ConfigureTools : public Command
{
public:
  ConfigureTools();
  Command* clone() const { return new ConfigureTools(*this); }

protected:
  void execute(Context* context);
};

ConfigureTools::ConfigureTools()
  : Command("configure_tools",
	    "Configure Tools",
	    CmdUIOnlyFlag)
{
}

void ConfigureTools::execute(Context* context)
{
  JWidget filled, tiled, tiled_x, tiled_y, use_grid, view_grid, set_grid;
  JWidget brush_size, brush_angle, glass_dirty;
  JWidget spray_width, air_speed;
  JWidget cursor_color, cursor_color_box;
  JWidget brush_preview_box;
  JWidget brush_type_box, brush_type;
  JWidget check_onionskin;
  JWidget brush_preview;
  bool first_time = false;

  if (!window) {
    window = static_cast<Frame*>(load_widget("toolconf.jid", "configure_tool"));
    first_time = true;
  }
  /* if the window is opened, close it */
  else if (jwidget_is_visible(window)) {
    window->closeWindow(NULL);
    return;
  }

  try {
    get_widgets(window,
		"filled", &filled,
		"tiled", &tiled,
		"tiled_x", &tiled_x,
		"tiled_y", &tiled_y,
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
		"onionskin", &check_onionskin, NULL);
  }
  catch (...) {
    jwidget_free(window);
    window = NULL;
    throw;
  }

  /* cursor-color */
  if (first_time) {
    cursor_color = colorbutton_new(get_cursor_color(), IMAGE_INDEXED);
    cursor_color->setName("cursor_color");
  }
  else {
    cursor_color = jwidget_find_name(window, "cursor_color");
  }

  /* brush-preview */
  if (first_time) {
    brush_preview = new Widget(JI_WIDGET);
    brush_preview->min_w = 32 + 4;
    brush_preview->min_h = 32 + 4;

    brush_preview->setName("brush_preview");
    jwidget_add_hook(brush_preview, JI_WIDGET,
		     brush_preview_msg_proc, NULL);
  }
  else {
    brush_preview = jwidget_find_name(window, "brush_preview");
  }

  /* brush-type */
  if (first_time) {
    brush_type = group_button_new(3, 1, get_brush_type(),
				  GFX_BRUSH_CIRCLE,
				  GFX_BRUSH_SQUARE,
				  GFX_BRUSH_LINE);

    brush_type->setName("brush_type");
  }
  else {
    brush_type = jwidget_find_name(window, "brush_type");
  }

  if (get_filled_mode()) jwidget_select(filled);
  if (get_tiled_mode() != TILED_NONE) {
    jwidget_select(tiled);
    if (get_tiled_mode() & TILED_X_AXIS) jwidget_select(tiled_x);
    if (get_tiled_mode() & TILED_Y_AXIS) jwidget_select(tiled_y);
  }
  if (get_use_grid()) jwidget_select(use_grid);
  if (get_view_grid()) jwidget_select(view_grid);
  jslider_set_value(brush_size, get_brush_size());
  jslider_set_value(brush_angle, get_brush_angle());
  jslider_set_value(glass_dirty, get_glass_dirty());
  jslider_set_value(spray_width, get_spray_width());
  jslider_set_value(air_speed, get_air_speed());
  if (get_onionskin()) jwidget_select(check_onionskin);

  if (first_time) {
    /* append children */
    jwidget_add_child(cursor_color_box, cursor_color);
    jwidget_add_child(brush_preview_box, brush_preview);
    jwidget_add_child(brush_type_box, brush_type);

    /* append hooks */
    HOOK(window, JI_SIGNAL_WINDOW_CLOSE, window_close_hook, 0);
    HOOK(filled, JI_SIGNAL_CHECK_CHANGE, filled_check_change_hook, 0);
    HOOK(tiled, JI_SIGNAL_CHECK_CHANGE, tiled_check_change_hook, 0);
    HOOK(tiled_x, JI_SIGNAL_CHECK_CHANGE, tiled_xy_check_change_hook, (void*)TILED_X_AXIS);
    HOOK(tiled_y, JI_SIGNAL_CHECK_CHANGE, tiled_xy_check_change_hook, (void*)TILED_Y_AXIS);
    HOOK(use_grid, JI_SIGNAL_CHECK_CHANGE, use_grid_check_change_hook, 0);
    HOOK(view_grid, JI_SIGNAL_CHECK_CHANGE, view_grid_check_change_hook, 0);
    HOOK(set_grid, JI_SIGNAL_BUTTON_SELECT, set_grid_button_select_hook, 0);
    HOOK(brush_size, JI_SIGNAL_SLIDER_CHANGE, brush_size_slider_change_hook, brush_preview);
    HOOK(brush_angle, JI_SIGNAL_SLIDER_CHANGE, brush_angle_slider_change_hook, brush_preview);
    HOOK(brush_type, SIGNAL_GROUP_BUTTON_CHANGE, brush_type_change_hook, brush_preview);
    HOOK(glass_dirty, JI_SIGNAL_SLIDER_CHANGE, glass_dirty_slider_change_hook, 0);
    HOOK(air_speed, JI_SIGNAL_SLIDER_CHANGE, air_speed_slider_change_hook, 0);
    HOOK(spray_width, JI_SIGNAL_SLIDER_CHANGE, spray_width_slider_change_hook, 0);
    HOOK(cursor_color, SIGNAL_COLORBUTTON_CHANGE, cursor_button_change_hook, 0);
    HOOK(check_onionskin, JI_SIGNAL_CHECK_CHANGE, onionskin_check_change_hook, 0);

    App::instance()->add_hook(AppEvent::Exit, new FreeWidget(window));
  }

  /* default position */
  window->remap_window();
  window->center_window();

  /* load window configuration */
  load_window_pos(window, "ConfigureTool");

  window->open_window_bg();
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
      return true;
    }
  }

  return false;
}

static bool window_close_hook(JWidget widget, void *data)
{
  /* isn't running anymore */
/*   window = NULL; */

  /* save window configuration */
  save_window_pos(widget, "ConfigureTool");

  return false;
}

static bool brush_size_slider_change_hook(JWidget widget, void *data)
{
  set_brush_size(jslider_get_value(widget));
  jwidget_dirty((JWidget)data);
  return false;
}

static bool brush_angle_slider_change_hook(JWidget widget, void *data)
{
  set_brush_angle(jslider_get_value(widget));
  jwidget_dirty((JWidget)data);
  return false;
}

static bool brush_type_change_hook(JWidget widget, void *data)
{
  int type = group_button_get_selected(widget);

  set_brush_type(type);
  jwidget_dirty((JWidget)data);

  statusbar_set_text(app_get_statusbar(), 250,
		      "Brush type: %s",
		      type == BRUSH_CIRCLE ? "Circle":
		      type == BRUSH_SQUARE ? "Square":
		      type == BRUSH_LINE ? "Line": "Unknown");

  return true;
}

static bool glass_dirty_slider_change_hook(JWidget widget, void *data)
{
  set_glass_dirty(jslider_get_value(widget));
  return false;
}

static bool spray_width_slider_change_hook(JWidget widget, void *data)
{
  set_spray_width(jslider_get_value(widget));
  return false;
}

static bool air_speed_slider_change_hook(JWidget widget, void *data)
{
  set_air_speed(jslider_get_value(widget));
  return false;
}

static bool filled_check_change_hook(JWidget widget, void *data)
{
  set_filled_mode(jwidget_is_selected(widget));
  return false;
}

static bool tiled_check_change_hook(JWidget widget, void *data)
{
  bool flag = jwidget_is_selected(widget);
  set_tiled_mode(flag ? TILED_BOTH: TILED_NONE);
  widget->findSibling("tiled_x")->setSelected(flag);
  widget->findSibling("tiled_y")->setSelected(flag);
  return false;
}

static bool tiled_xy_check_change_hook(JWidget widget, void *data)
{
  int tiled_axis = (int)((size_t)data);
  int tiled_mode = get_tiled_mode();

  if (jwidget_is_selected(widget))
    tiled_mode |= tiled_axis;
  else
    tiled_mode &= ~tiled_axis;

  widget->findSibling("tiled")->setSelected(tiled_mode != TILED_NONE);

  set_tiled_mode((tiled_t)tiled_mode);
  return false;
}

static bool use_grid_check_change_hook(JWidget widget, void *data)
{
  set_use_grid(jwidget_is_selected(widget));
  return false;
}

static bool view_grid_check_change_hook(JWidget widget, void *data)
{
  set_view_grid(jwidget_is_selected(widget));
  refresh_all_editors();
  return false;
}

static bool set_grid_button_select_hook(JWidget widget, void *data)
{
  try {
    // TODO use the same context as in ConfigureTools::execute
    const CurrentSpriteReader sprite(UIContext::instance());

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
	       "<<You have to select a sprite with mask."
	       "<<The boundaries of the mask will be used"
	       "<<to specify the grid area.||&OK"));
    }
  }
  catch (locked_sprite_exception& e) {
    e.show();
  }

  return true;
}

static bool cursor_button_change_hook(JWidget widget, void *data)
{
  set_cursor_color(colorbutton_get_color(widget));
  return true;
}

static bool onionskin_check_change_hook(JWidget widget, void *data)
{
  set_onionskin(jwidget_is_selected(widget));
  refresh_all_editors();
  return false;
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_configure_tools_command()
{
  return new ConfigureTools;
}
