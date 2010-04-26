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
#include "Vaca/Bind.h"

#include "app.h"
#include "commands/command.h"
#include "commands/commands.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/rootmenu.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/pen.h"
#include "raster/sprite.h"
#include "settings/settings.h"
#include "sprite_wrappers.h"
#include "ui_context.h"
#include "widgets/colbut.h"
#include "widgets/editor.h"
#include "widgets/groupbut.h"
#include "widgets/statebar.h"

static Frame* window = NULL;

static bool brush_preview_msg_proc(JWidget widget, JMessage msg);

static bool window_close_hook(JWidget widget, void *data);
static bool brush_size_slider_change_hook(JWidget widget, void *data);
static bool brush_angle_slider_change_hook(JWidget widget, void *data);
static bool brush_type_change_hook(JWidget widget, void *data);
static bool opacity_slider_change_hook(JWidget widget, void *data);
static bool spray_width_slider_change_hook(JWidget widget, void *data);
static bool air_speed_slider_change_hook(JWidget widget, void *data);
static bool tiled_check_change_hook(JWidget widget, void *data);
static bool tiled_xy_check_change_hook(JWidget widget, void *data);
static bool snap_to_grid_check_change_hook(JWidget widget, void *data);
static bool view_grid_check_change_hook(JWidget widget, void *data);
static bool set_grid_button_select_hook(JWidget widget, void *data);
static bool onionskin_check_change_hook(JWidget widget, void *data);

// Slot for App::Exit signal 
static void on_exit_delete_this_widget()
{
  assert(window != NULL);
  jwidget_free(window);
}

static void on_pen_size_after_change()
{
  Widget* brush_size = window->findChild("brush_size");
  Widget* brush_preview = window->findChild("brush_preview");

  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  IToolSettings* tool_settings = UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool);

  jslider_set_value(brush_size, tool_settings->getPen()->getSize());

  // Regenerate the preview
  brush_preview->dirty();
}

static void on_current_tool_change()
{
  Widget* brush_size = window->findChild("brush_size");
  Widget* brush_angle = window->findChild("brush_angle");
  Widget* brush_type = window->findChild("brush_type");
  Widget* brush_preview = window->findChild("brush_preview");
  Widget* opacity = window->findChild("opacity");
  Widget* spray_width = window->findChild("spray_width");
  Widget* air_speed = window->findChild("air_speed");

  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  IToolSettings* tool_settings = UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool);

  jslider_set_value(opacity, tool_settings->getOpacity());
  jslider_set_value(brush_size, tool_settings->getPen()->getSize());
  jslider_set_value(brush_angle, tool_settings->getPen()->getAngle());
  jslider_set_value(spray_width, tool_settings->getSprayWidth());
  jslider_set_value(air_speed, tool_settings->getSpraySpeed());

  group_button_select(brush_type, tool_settings->getPen()->getType());

  // Regenerate the preview
  brush_preview->dirty();
}

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
  JWidget tiled, tiled_x, tiled_y, snap_to_grid, view_grid, set_grid;
  JWidget brush_size, brush_angle, opacity;
  JWidget spray_width, air_speed;
  JWidget brush_preview_box;
  JWidget brush_type_box, brush_type;
  JWidget check_onionskin;
  JWidget brush_preview;
  bool first_time = false;

  if (!window) {
    window = static_cast<Frame*>(load_widget("tools_configuration.xml", "configure_tool"));
    first_time = true;
  }
  /* if the window is opened, close it */
  else if (jwidget_is_visible(window)) {
    window->closeWindow(NULL);
    return;
  }

  try {
    get_widgets(window,
		"tiled", &tiled,
		"tiled_x", &tiled_x,
		"tiled_y", &tiled_y,
		"snap_to_grid", &snap_to_grid,
		"view_grid", &view_grid,
		"set_grid", &set_grid,
		"brush_size", &brush_size,
		"brush_angle", &brush_angle,
		"opacity", &opacity,
		"spray_width", &spray_width,
		"air_speed", &air_speed,
		"brush_preview_box", &brush_preview_box,
		"brush_type_box", &brush_type_box,
		"onionskin", &check_onionskin, NULL);
  }
  catch (...) {
    jwidget_free(window);
    window = NULL;
    throw;
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

  // Current settings
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* current_tool = settings->getCurrentTool();
  IToolSettings* tool_settings = settings->getToolSettings(current_tool);

  /* brush-type */
  if (first_time) {
    PenType type = tool_settings->getPen()->getType();

    brush_type = group_button_new(3, 1, type,
				  GFX_BRUSH_CIRCLE,
				  GFX_BRUSH_SQUARE,
				  GFX_BRUSH_LINE);

    brush_type->setName("brush_type");
  }
  else {
    brush_type = window->findChild("brush_type");
  }

  if (settings->getTiledMode() != TILED_NONE) {
    jwidget_select(tiled);
    if (settings->getTiledMode() & TILED_X_AXIS) jwidget_select(tiled_x);
    if (settings->getTiledMode() & TILED_Y_AXIS) jwidget_select(tiled_y);
  }
      
  if (settings->getSnapToGrid()) jwidget_select(snap_to_grid);
  if (settings->getGridVisible()) jwidget_select(view_grid);
  if (settings->getUseOnionskin()) jwidget_select(check_onionskin);

  if (first_time) {
    // Append children
    jwidget_add_child(brush_preview_box, brush_preview);
    jwidget_add_child(brush_type_box, brush_type);

    // Append hooks
    window->Close.connect(Vaca::Bind<bool>(&window_close_hook, (JWidget)window, (void*)0));
    HOOK(tiled, JI_SIGNAL_CHECK_CHANGE, tiled_check_change_hook, 0);
    HOOK(tiled_x, JI_SIGNAL_CHECK_CHANGE, tiled_xy_check_change_hook, (void*)TILED_X_AXIS);
    HOOK(tiled_y, JI_SIGNAL_CHECK_CHANGE, tiled_xy_check_change_hook, (void*)TILED_Y_AXIS);
    HOOK(snap_to_grid, JI_SIGNAL_CHECK_CHANGE, snap_to_grid_check_change_hook, 0);
    HOOK(view_grid, JI_SIGNAL_CHECK_CHANGE, view_grid_check_change_hook, 0);
    HOOK(set_grid, JI_SIGNAL_BUTTON_SELECT, set_grid_button_select_hook, 0);
    HOOK(brush_size, JI_SIGNAL_SLIDER_CHANGE, brush_size_slider_change_hook, brush_preview);
    HOOK(brush_angle, JI_SIGNAL_SLIDER_CHANGE, brush_angle_slider_change_hook, brush_preview);
    HOOK(brush_type, SIGNAL_GROUP_BUTTON_CHANGE, brush_type_change_hook, brush_preview);
    HOOK(opacity, JI_SIGNAL_SLIDER_CHANGE, opacity_slider_change_hook, 0);
    HOOK(air_speed, JI_SIGNAL_SLIDER_CHANGE, air_speed_slider_change_hook, 0);
    HOOK(spray_width, JI_SIGNAL_SLIDER_CHANGE, spray_width_slider_change_hook, 0);
    HOOK(check_onionskin, JI_SIGNAL_CHECK_CHANGE, onionskin_check_change_hook, 0);

    App::instance()->Exit.connect(&on_exit_delete_this_widget);
    App::instance()->PenSizeAfterChange.connect(&on_pen_size_after_change);
    App::instance()->CurrentToolChange.connect(&on_current_tool_change);
  }

  // Update current pen properties
  on_current_tool_change();

  // Default position
  window->remap_window();
  window->center_window();

  // Load window configuration
  load_window_pos(window, "ConfigureTool");

  window->open_window_bg();
}

static bool brush_preview_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_DRAW: {
      BITMAP *bmp = create_bitmap(jrect_w(widget->rc),
				  jrect_h(widget->rc));

      Tool* current_tool = UIContext::instance()
	->getSettings()
	->getCurrentTool();

      IPenSettings* pen_settings = UIContext::instance()
	->getSettings()
	->getToolSettings(current_tool)
	->getPen();

      assert(pen_settings != NULL);

      Pen* pen = new Pen(pen_settings->getType(),
			 pen_settings->getSize(),
			 pen_settings->getAngle());

      clear_to_color(bmp, makecol(0, 0, 0));
      image_to_allegro(pen->get_image(), bmp,
		       bmp->w/2 - pen->get_size()/2,
		       bmp->h/2 - pen->get_size()/2, NULL);
      blit(bmp, ji_screen, 0, 0, widget->rc->x1, widget->rc->y1,
	   bmp->w, bmp->h);
      destroy_bitmap(bmp);

      delete pen;
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
  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool)
    ->getPen()
    ->setSize(jslider_get_value(widget));

  jwidget_dirty((JWidget)data);
  return false;
}

static bool brush_angle_slider_change_hook(JWidget widget, void *data)
{
  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool)
    ->getPen()
    ->setAngle(jslider_get_value(widget));

  jwidget_dirty((JWidget)data);
  return false;
}

static bool brush_type_change_hook(JWidget widget, void *data)
{
  PenType type = (PenType)group_button_get_selected(widget);

  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool)
    ->getPen()
    ->setType(type);

  jwidget_dirty((JWidget)data);

  app_get_statusbar()
    ->setStatusText(250,
		    "Pen shape: %s",
		    type == PEN_TYPE_CIRCLE ? "Circle":
		    type == PEN_TYPE_SQUARE ? "Square":
		    type == PEN_TYPE_LINE ? "Line": "Unknown");

  return true;
}

static bool opacity_slider_change_hook(JWidget widget, void *data)
{
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* current_tool = settings->getCurrentTool();
  settings->getToolSettings(current_tool)->setOpacity(jslider_get_value(widget));
  return false;
}

static bool spray_width_slider_change_hook(JWidget widget, void *data)
{
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* current_tool = settings->getCurrentTool();
  settings->getToolSettings(current_tool)->setSprayWidth(jslider_get_value(widget));
  return false;
}

static bool air_speed_slider_change_hook(JWidget widget, void *data)
{
  ISettings* settings = UIContext::instance()->getSettings();
  Tool* current_tool = settings->getCurrentTool();
  settings->getToolSettings(current_tool)->setSpraySpeed(jslider_get_value(widget));
  return false;
}

static bool tiled_check_change_hook(JWidget widget, void *data)
{
  bool flag = jwidget_is_selected(widget);

  UIContext::instance()->getSettings()->setTiledMode(flag ? TILED_BOTH: TILED_NONE);

  widget->findSibling("tiled_x")->setSelected(flag);
  widget->findSibling("tiled_y")->setSelected(flag);
  return false;
}

static bool tiled_xy_check_change_hook(JWidget widget, void *data)
{
  int tiled_axis = (int)((size_t)data);
  int tiled_mode = UIContext::instance()->getSettings()->getTiledMode();

  if (jwidget_is_selected(widget))
    tiled_mode |= tiled_axis;
  else
    tiled_mode &= ~tiled_axis;

  widget->findSibling("tiled")->setSelected(tiled_mode != TILED_NONE);

  UIContext::instance()->getSettings()->setTiledMode((TiledMode)tiled_mode);
  return false;
}

static bool snap_to_grid_check_change_hook(JWidget widget, void *data)
{
  UIContext::instance()->getSettings()->setSnapToGrid(jwidget_is_selected(widget));
  return false;
}

static bool view_grid_check_change_hook(JWidget widget, void *data)
{
  UIContext::instance()->getSettings()->setGridVisible(jwidget_is_selected(widget));
  refresh_all_editors();
  return false;
}

static bool set_grid_button_select_hook(JWidget widget, void *data)
{
  try {
    // TODO use the same context as in ConfigureTools::execute
    const CurrentSpriteReader sprite(UIContext::instance());

    if (sprite && sprite->getMask() && sprite->getMask()->bitmap) {
      Rect bounds(sprite->getMask()->x, sprite->getMask()->y,
		  sprite->getMask()->w, sprite->getMask()->h);

      UIContext::instance()->getSettings()->setGridBounds(bounds);

      if (UIContext::instance()->getSettings()->getGridVisible())
	refresh_all_editors();
    }
    else {
      Command* grid_settings_cmd = 
	CommandsModule::instance()->get_command_by_name(CommandId::grid_settings);
  
      UIContext::instance()->execute_command(grid_settings_cmd, NULL);
    }
  }
  catch (locked_sprite_exception& e) {
    e.show();
  }
  return true;
}

static bool onionskin_check_change_hook(JWidget widget, void *data)
{
  UIContext::instance()->getSettings()->setUseOnionskin(jwidget_is_selected(widget));
  refresh_all_editors();
  return false;
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_configure_tools_command()
{
  return new ConfigureTools;
}
