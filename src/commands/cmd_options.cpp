/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "base/bind.h"
#include "gui/gui.h"

#include "app.h"
#include "commands/command.h"
#include "context.h"
#include "ini_file.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "util/render.h"
#include "widgets/color_button.h"
#include "widgets/editor/editor.h"

//////////////////////////////////////////////////////////////////////
// options

class OptionsCommand : public Command
{
public:
  OptionsCommand();
  Command* clone() { return new OptionsCommand(*this); }

protected:
  void onExecute(Context* context);

private:
  void onResetCheckedBg();

  ComboBox* m_checked_bg;
  Widget* m_checked_bg_zoom;
  ColorButton* m_checked_bg_color1;
  ColorButton* m_checked_bg_color2;
};

OptionsCommand::OptionsCommand()
  : Command("Options",
            "Options",
            CmdUIOnlyFlag)
{
}

void OptionsCommand::onExecute(Context* context)
{
  JWidget check_smooth;
  ColorButton* cursor_color;
  ColorButton* grid_color;
  ColorButton* pixel_grid_color;
  Widget* cursor_color_box;
  Widget* grid_color_box;
  Widget* pixel_grid_color_box;
  Button* button_ok;
  Widget* move_click2, *draw_click2;
  Button* checked_bg_reset;
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
              "checked_bg_size", &m_checked_bg,
              "checked_bg_zoom", &m_checked_bg_zoom,
              "checked_bg_color1_box", &checked_bg_color1_box,
              "checked_bg_color2_box", &checked_bg_color2_box,
              "checked_bg_reset", &checked_bg_reset,
              "undo_size_limit", &undo_size_limit,
              "button_ok", &button_ok, NULL);

  // Cursor color
  cursor_color = new ColorButton(Editor::get_cursor_color(), IMAGE_RGB);
  cursor_color->setName("cursor_color");
  cursor_color_box->addChild(cursor_color);

  // Grid color
  grid_color = new ColorButton(context->getSettings()->getGridColor(), IMAGE_RGB);
  grid_color->setName("grid_color");
  grid_color_box->addChild(grid_color);

  // Pixel grid color
  pixel_grid_color = new ColorButton(context->getSettings()->getPixelGridColor(), IMAGE_RGB);
  pixel_grid_color->setName("pixel_grid_color");
  pixel_grid_color_box->addChild(pixel_grid_color);

  // Others
  if (get_config_bool("Options", "MoveClick2", false))
    move_click2->setSelected(true);

  if (get_config_bool("Options", "DrawClick2", false))
    draw_click2->setSelected(true);

  if (get_config_bool("Options", "MoveSmooth", true))
    check_smooth->setSelected(true);

  // Checked background size
  m_checked_bg->addItem("16x16");
  m_checked_bg->addItem("8x8");
  m_checked_bg->addItem("4x4");
  m_checked_bg->addItem("2x2");
  m_checked_bg->setSelectedItem((int)RenderEngine::getCheckedBgType());

  // Zoom checked background
  if (RenderEngine::getCheckedBgZoom())
    m_checked_bg_zoom->setSelected(true);

  // Checked background colors
  m_checked_bg_color1 = new ColorButton(RenderEngine::getCheckedBgColor1(), IMAGE_RGB);
  m_checked_bg_color2 = new ColorButton(RenderEngine::getCheckedBgColor2(), IMAGE_RGB);

  checked_bg_color1_box->addChild(m_checked_bg_color1);
  checked_bg_color2_box->addChild(m_checked_bg_color2);

  // Reset button
  checked_bg_reset->Click.connect(Bind<void>(&OptionsCommand::onResetCheckedBg, this));

  // Undo limit
  undo_size_limit->setTextf("%d", get_config_int("Options", "UndoSizeLimit", 8));

  // Show the window and wait the user to close it
  window->open_window_fg();

  if (window->get_killer() == button_ok) {
    int undo_size_limit_value;

    Editor::set_cursor_color(cursor_color->getColor());
    context->getSettings()->setGridColor(grid_color->getColor());
    context->getSettings()->setPixelGridColor(pixel_grid_color->getColor());

    set_config_bool("Options", "MoveSmooth", check_smooth->isSelected());
    set_config_bool("Options", "MoveClick2", move_click2->isSelected());
    set_config_bool("Options", "DrawClick2", draw_click2->isSelected());

    RenderEngine::setCheckedBgType((RenderEngine::CheckedBgType)m_checked_bg->getSelectedItem());
    RenderEngine::setCheckedBgZoom(m_checked_bg_zoom->isSelected());
    RenderEngine::setCheckedBgColor1(m_checked_bg_color1->getColor());
    RenderEngine::setCheckedBgColor2(m_checked_bg_color2->getColor());

    undo_size_limit_value = undo_size_limit->getTextInt();
    undo_size_limit_value = MID(1, undo_size_limit_value, 9999);
    set_config_int("Options", "UndoSizeLimit", undo_size_limit_value);

    // Save configuration
    flush_config_file();

    // Refresh all editors
    refresh_all_editors();
  }
}

void OptionsCommand::onResetCheckedBg()
{
  // Default values
  m_checked_bg->setSelectedItem((int)RenderEngine::CHECKED_BG_16X16);
  m_checked_bg_zoom->setSelected(true);
  m_checked_bg_color1->setColor(Color::fromRgb(128, 128, 128));
  m_checked_bg_color2->setColor(Color::fromRgb(192, 192, 192));
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createOptionsCommand()
{
  return new OptionsCommand;
}
