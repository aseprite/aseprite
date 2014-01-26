/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context.h"
#include "app/find_widget.h"
#include "app/ini_file.h"
#include "app/load_widget.h"
#include "app/modules/editors.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/ui/color_button.h"
#include "app/ui/editor/editor.h"
#include "app/util/render.h"
#include "base/bind.h"
#include "raster/image.h"
#include "ui/ui.h"

namespace app {

using namespace ui;

class OptionsCommand : public Command {
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
  // Load the window widget
  base::UniquePtr<Window> window(app::load_widget<Window>("options.xml", "options"));
  Widget* check_smooth = app::find_widget<Widget>(window, "smooth");
  Widget* check_autotimeline = app::find_widget<Widget>(window, "autotimeline");
  Widget* show_scrollbars = app::find_widget<Widget>(window, "show_scrollbars");
  Widget* move_click2 = app::find_widget<Widget>(window, "move_click2");
  Widget* draw_click2 = app::find_widget<Widget>(window, "draw_click2");
  Widget* cursor_color_box = app::find_widget<Widget>(window, "cursor_color_box");
  Widget* grid_color_box = app::find_widget<Widget>(window, "grid_color_box");
  Widget* pixel_grid_color_box = app::find_widget<Widget>(window, "pixel_grid_color_box");
  m_checked_bg = app::find_widget<ComboBox>(window, "checked_bg_size");
  m_checked_bg_zoom = app::find_widget<Widget>(window, "checked_bg_zoom");
  Widget* checked_bg_color1_box = app::find_widget<Widget>(window, "checked_bg_color1_box");
  Widget* checked_bg_color2_box = app::find_widget<Widget>(window, "checked_bg_color2_box");
  Button* checked_bg_reset = app::find_widget<Button>(window, "checked_bg_reset");
  Widget* undo_size_limit = app::find_widget<Widget>(window, "undo_size_limit");
  Widget* undo_goto_modified = app::find_widget<Widget>(window, "undo_goto_modified");
  Widget* button_ok = app::find_widget<Widget>(window, "button_ok");

  // Cursor color
  ColorButton* cursor_color = new ColorButton(Editor::get_cursor_color(), IMAGE_RGB);
  cursor_color->setId("cursor_color");
  cursor_color_box->addChild(cursor_color);

  // Get global settings for documents
  ISettings* settings = context->settings();
  IDocumentSettings* docSettings = settings->getDocumentSettings(NULL);

  // Grid color
  ColorButton* grid_color = new ColorButton(docSettings->getGridColor(), IMAGE_RGB);
  grid_color->setId("grid_color");
  grid_color_box->addChild(grid_color);

  // Pixel grid color
  ColorButton* pixel_grid_color = new ColorButton(docSettings->getPixelGridColor(), IMAGE_RGB);
  pixel_grid_color->setId("pixel_grid_color");
  pixel_grid_color_box->addChild(pixel_grid_color);

  // Others
  if (get_config_bool("Options", "MoveClick2", false))
    move_click2->setSelected(true);

  if (get_config_bool("Options", "DrawClick2", false))
    draw_click2->setSelected(true);

  if (get_config_bool("Options", "MoveSmooth", true))
    check_smooth->setSelected(true);

  if (get_config_bool("Options", "AutoShowTimeline", true))
    check_autotimeline->setSelected(true);

  if (settings->getShowSpriteEditorScrollbars())
    show_scrollbars->setSelected(true);

  // Checked background size
  m_checked_bg->addItem("16x16");
  m_checked_bg->addItem("8x8");
  m_checked_bg->addItem("4x4");
  m_checked_bg->addItem("2x2");
  m_checked_bg->setSelectedItemIndex((int)RenderEngine::getCheckedBgType());

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

  // Goto modified frame/layer on undo/redo
  if (get_config_bool("Options", "UndoGotoModified", true))
    undo_goto_modified->setSelected(true);

  // Show the window and wait the user to close it
  window->openWindowInForeground();

  if (window->getKiller() == button_ok) {
    int undo_size_limit_value;

    Editor::set_cursor_color(cursor_color->getColor());
    docSettings->setGridColor(grid_color->getColor());
    docSettings->setPixelGridColor(pixel_grid_color->getColor());

    set_config_bool("Options", "MoveSmooth", check_smooth->isSelected());
    set_config_bool("Options", "AutoShowTimeline", check_autotimeline->isSelected());
    set_config_bool("Options", "MoveClick2", move_click2->isSelected());
    set_config_bool("Options", "DrawClick2", draw_click2->isSelected());

    settings->setShowSpriteEditorScrollbars(show_scrollbars->isSelected());

    RenderEngine::setCheckedBgType((RenderEngine::CheckedBgType)m_checked_bg->getSelectedItemIndex());
    RenderEngine::setCheckedBgZoom(m_checked_bg_zoom->isSelected());
    RenderEngine::setCheckedBgColor1(m_checked_bg_color1->getColor());
    RenderEngine::setCheckedBgColor2(m_checked_bg_color2->getColor());

    undo_size_limit_value = undo_size_limit->getTextInt();
    undo_size_limit_value = MID(1, undo_size_limit_value, 9999);
    set_config_int("Options", "UndoSizeLimit", undo_size_limit_value);
    set_config_bool("Options", "UndoGotoModified", undo_goto_modified->isSelected());

    // Save configuration
    flush_config_file();
  }
}

void OptionsCommand::onResetCheckedBg()
{
  // Default values
  m_checked_bg->setSelectedItemIndex((int)RenderEngine::CHECKED_BG_16X16);
  m_checked_bg_zoom->setSelected(true);
  m_checked_bg_color1->setColor(app::Color::fromRgb(128, 128, 128));
  m_checked_bg_color2->setColor(app::Color::fromRgb(192, 192, 192));
}

Command* CommandFactory::createOptionsCommand()
{
  return new OptionsCommand;
}

} // namespace app
