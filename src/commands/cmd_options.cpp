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

#include "app.h"
#include "commands/command.h"
#include "context.h"
#include "core/cfg.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "util/render.h"
#include "widgets/colbut.h"
#include "widgets/editor.h"

//////////////////////////////////////////////////////////////////////
// options

// TODO make these variables member of OptionsCommand
static ComboBox* checked_bg;
static Widget* checked_bg_zoom;
static Widget* checked_bg_color1;
static Widget* checked_bg_color2;

// TODO make this a member of OptionsCommand when button signal is converted to Vaca::Signal
static bool checked_bg_reset_hook(JWidget widget, void *data);

class OptionsCommand : public Command
{
public:
  OptionsCommand();
  Command* clone() { return new OptionsCommand(*this); }

protected:
  void execute(Context* context);
};

OptionsCommand::OptionsCommand()
  : Command("options",
	    "Options",
	    CmdUIOnlyFlag)
{
}

void OptionsCommand::execute(Context* context)
{
  JWidget check_smooth;
  JWidget cursor_color, cursor_color_box;
  JWidget grid_color, grid_color_box;
  JWidget pixel_grid_color, pixel_grid_color_box;
  JWidget button_ok;
  JWidget move_click2, draw_click2;
  JWidget checked_bg_reset;
  JWidget checked_bg_color1_box, checked_bg_color2_box;
  JWidget undo_size_limit;

  /* load the window widget */
  FramePtr window(load_widget("options.xml", "options"));
  get_widgets(window,
	      "smooth", &check_smooth,
	      "move_click2", &move_click2,
	      "draw_click2", &draw_click2,
	      "cursor_color_box", &cursor_color_box,
	      "grid_color_box", &grid_color_box,
	      "pixel_grid_color_box", &pixel_grid_color_box,
	      "checked_bg_size", &checked_bg,
	      "checked_bg_zoom", &checked_bg_zoom,
	      "checked_bg_color1_box", &checked_bg_color1_box,
	      "checked_bg_color2_box", &checked_bg_color2_box,
	      "checked_bg_reset", &checked_bg_reset,
	      "undo_size_limit", &undo_size_limit,
	      "button_ok", &button_ok, NULL);

  // Cursor color
  cursor_color = colorbutton_new(Editor::get_cursor_color(), IMAGE_RGB);
  cursor_color->setName("cursor_color");
  jwidget_add_child(cursor_color_box, cursor_color);

  // Grid color
  grid_color = colorbutton_new(context->getSettings()->getGridColor(), IMAGE_RGB);
  grid_color->setName("grid_color");
  jwidget_add_child(grid_color_box, grid_color);

  // Pixel grid color
  pixel_grid_color = colorbutton_new(context->getSettings()->getPixelGridColor(), IMAGE_RGB);
  pixel_grid_color->setName("pixel_grid_color");
  jwidget_add_child(pixel_grid_color_box, pixel_grid_color);

  // Others
  if (get_config_bool("Options", "MoveClick2", false))
    jwidget_select(move_click2);

  if (get_config_bool("Options", "DrawClick2", false))
    jwidget_select(draw_click2);

  if (get_config_bool("Options", "MoveSmooth", true))
    jwidget_select(check_smooth);

  // Checked background size
  checked_bg->addItem("16x16");
  checked_bg->addItem("8x8");
  checked_bg->addItem("4x4");
  checked_bg->addItem("2x2");
  checked_bg->setSelectedItem((int)RenderEngine::getCheckedBgType());

  // Zoom checked background
  if (RenderEngine::getCheckedBgZoom())
    jwidget_select(checked_bg_zoom);

  // Checked background colors
  checked_bg_color1 = colorbutton_new(RenderEngine::getCheckedBgColor1(), IMAGE_RGB);
  checked_bg_color2 = colorbutton_new(RenderEngine::getCheckedBgColor2(), IMAGE_RGB);

  jwidget_add_child(checked_bg_color1_box, checked_bg_color1);
  jwidget_add_child(checked_bg_color2_box, checked_bg_color2);

  // Reset button
  HOOK(checked_bg_reset, JI_SIGNAL_BUTTON_SELECT, checked_bg_reset_hook, 0);

  // Undo limit
  undo_size_limit->setTextf("%d", get_config_int("Options", "UndoSizeLimit", 8));

  // Show the window and wait the user to close it
  window->open_window_fg();

  if (window->get_killer() == button_ok) {
    int undo_size_limit_value;

    Editor::set_cursor_color(colorbutton_get_color(cursor_color));
    context->getSettings()->setGridColor(colorbutton_get_color(grid_color));
    context->getSettings()->setPixelGridColor(colorbutton_get_color(pixel_grid_color));

    set_config_bool("Options", "MoveSmooth", jwidget_is_selected(check_smooth));
    set_config_bool("Options", "MoveClick2", jwidget_is_selected(move_click2));
    set_config_bool("Options", "DrawClick2", jwidget_is_selected(draw_click2));

    RenderEngine::setCheckedBgType((RenderEngine::CheckedBgType)checked_bg->getSelectedItem());
    RenderEngine::setCheckedBgZoom(jwidget_is_selected(checked_bg_zoom));
    RenderEngine::setCheckedBgColor1(colorbutton_get_color(checked_bg_color1));
    RenderEngine::setCheckedBgColor2(colorbutton_get_color(checked_bg_color2));

    undo_size_limit_value = undo_size_limit->getTextInt();
    undo_size_limit_value = MID(1, undo_size_limit_value, 9999);
    set_config_int("Options", "UndoSizeLimit", undo_size_limit_value);

    // Save configuration
    flush_config_file();

    // Refresh all editors
    refresh_all_editors();
  }
}

static bool checked_bg_reset_hook(JWidget widget, void *data)
{
  // Default values
  checked_bg->setSelectedItem((int)RenderEngine::CHECKED_BG_16X16);
  jwidget_select(checked_bg_zoom);
  colorbutton_set_color(checked_bg_color1, color_rgb(128, 128, 128));
  colorbutton_set_color(checked_bg_color2, color_rgb(192, 192, 192));
  return true;
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::create_options_command()
{
  return new OptionsCommand;
}
