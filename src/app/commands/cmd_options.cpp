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
#include "app/launcher.h"
#include "app/load_widget.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
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

class OptionsWindow : public Window {
public:
  OptionsWindow(Context* context)
    : Window(Window::WithTitleBar, "Options")
    , m_settings(context->settings())
    , m_docSettings(m_settings->getDocumentSettings(NULL))
    , m_checked_bg_color1(new ColorButton(RenderEngine::getCheckedBgColor1(), IMAGE_RGB))
    , m_checked_bg_color2(new ColorButton(RenderEngine::getCheckedBgColor2(), IMAGE_RGB))
    , m_pixelGridColor(new ColorButton(m_docSettings->getPixelGridColor(), IMAGE_RGB))
    , m_gridColor(new ColorButton(m_docSettings->getGridColor(), IMAGE_RGB))
    , m_cursorColor(new ColorButton(Editor::get_cursor_color(), IMAGE_RGB))
  {
    addChild(app::load_widget<Box>("options.xml", "main_box"));

    // Load the this widget
    app::finder(this)
      >> "section_listbox" >> m_section_listbox
      >> "panel" >> m_panel
      >> "smooth" >> m_check_smooth
      >> "autotimeline" >> m_check_autotimeline
      >> "show_scrollbars" >> m_show_scrollbars
      >> "cursor_color_box" >> m_cursor_color_box
      >> "grid_color_box" >> m_grid_color_box
      >> "pixel_grid_color_box" >> m_pixel_grid_color_box
      >> "checked_bg_size" >> m_checked_bg
      >> "checked_bg_zoom" >> m_checked_bg_zoom
      >> "checked_bg_color1_box" >> m_checked_bg_color1_box
      >> "checked_bg_color2_box" >> m_checked_bg_color2_box
      >> "checked_bg_reset" >> m_checked_bg_reset
      >> "undo_size_limit" >> m_undo_size_limit
      >> "undo_goto_modified" >> m_undo_goto_modified
      >> "screen_scale" >> m_screen_scale
      >> "locate_file" >> m_locate_file
      >> "button_ok" >> m_button_ok;

    m_section_listbox->ChangeSelectedItem.connect(Bind<void>(&OptionsWindow::onChangeSection, this));
    m_cursor_color_box->addChild(m_cursorColor);

    // Grid color
    m_gridColor->setId("grid_color");
    m_grid_color_box->addChild(m_gridColor);

    // Pixel grid color
    m_pixelGridColor->setId("pixel_grid_color");
    m_pixel_grid_color_box->addChild(m_pixelGridColor);

    // Others
    if (get_config_bool("Options", "MoveSmooth", true))
      m_check_smooth->setSelected(true);

    if (get_config_bool("Options", "AutoShowTimeline", true))
      m_check_autotimeline->setSelected(true);

    if (m_settings->getShowSpriteEditorScrollbars())
      m_show_scrollbars->setSelected(true);

    // Checked background size
    m_screen_scale->addItem("1:1");
    m_screen_scale->addItem("2:1");
    m_screen_scale->addItem("3:1");
    m_screen_scale->addItem("4:1");
    m_screen_scale->setSelectedItemIndex(get_screen_scaling()-1);

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
    m_checked_bg_color1_box->addChild(m_checked_bg_color1);
    m_checked_bg_color2_box->addChild(m_checked_bg_color2);

    // Reset button
    m_checked_bg_reset->Click.connect(Bind<void>(&OptionsWindow::onResetCheckedBg, this));

    // Locate config file
    m_locate_file->Click.connect(Bind<void>(&OptionsWindow::onLocateConfigFile, this));

    // Undo limit
    m_undo_size_limit->setTextf("%d", get_config_int("Options", "UndoSizeLimit", 8));

    // Goto modified frame/layer on undo/redo
    if (get_config_bool("Options", "UndoGotoModified", true))
      m_undo_goto_modified->setSelected(true);

    m_section_listbox->selectIndex(0);
  }

  bool ok() {
    return (getKiller() == m_button_ok);
  }

  void saveConfig() {
    Editor::set_cursor_color(m_cursorColor->getColor());
    m_docSettings->setGridColor(m_gridColor->getColor());
    m_docSettings->setPixelGridColor(m_pixelGridColor->getColor());

    set_config_bool("Options", "MoveSmooth", m_check_smooth->isSelected());
    set_config_bool("Options", "AutoShowTimeline", m_check_autotimeline->isSelected());

    m_settings->setShowSpriteEditorScrollbars(m_show_scrollbars->isSelected());

    RenderEngine::setCheckedBgType((RenderEngine::CheckedBgType)m_checked_bg->getSelectedItemIndex());
    RenderEngine::setCheckedBgZoom(m_checked_bg_zoom->isSelected());
    RenderEngine::setCheckedBgColor1(m_checked_bg_color1->getColor());
    RenderEngine::setCheckedBgColor2(m_checked_bg_color2->getColor());

    int undo_size_limit_value;
    undo_size_limit_value = m_undo_size_limit->getTextInt();
    undo_size_limit_value = MID(1, undo_size_limit_value, 9999);
    set_config_int("Options", "UndoSizeLimit", undo_size_limit_value);
    set_config_bool("Options", "UndoGotoModified", m_undo_goto_modified->isSelected());

    int new_screen_scaling = m_screen_scale->getSelectedItemIndex()+1;
    if (new_screen_scaling != get_screen_scaling()) {
      set_screen_scaling(new_screen_scaling);

      ui::Alert::show(PACKAGE
        "<<You must restart the program to see your changes to 'Screen Scale' setting."
        "||&OK");
    }

    // Save configuration
    flush_config_file();
  }

private:
  void onChangeSection() {
    ListItem* item = m_section_listbox->getSelectedChild();
    if (!item)
      return;

    m_panel->showChild(findChild(item->getValue().c_str()));
  }

  void onResetCheckedBg() {
    // Default values
    m_checked_bg->setSelectedItemIndex((int)RenderEngine::CHECKED_BG_16X16);
    m_checked_bg_zoom->setSelected(true);
    m_checked_bg_color1->setColor(app::Color::fromRgb(128, 128, 128));
    m_checked_bg_color2->setColor(app::Color::fromRgb(192, 192, 192));
  }

  void onLocateConfigFile() {
    app::launcher::open_folder(app::get_config_file());
  }

  ISettings* m_settings;
  IDocumentSettings* m_docSettings;
  ListBox* m_section_listbox;
  Panel* m_panel;
  Widget* m_check_smooth;
  Widget* m_check_autotimeline;
  Widget* m_show_scrollbars;
  Widget* m_cursor_color_box;
  Widget* m_grid_color_box;
  Widget* m_pixel_grid_color_box;
  ComboBox* m_checked_bg;
  Widget* m_checked_bg_zoom;
  Widget* m_checked_bg_color1_box;
  Widget* m_checked_bg_color2_box;
  Button* m_checked_bg_reset;
  Widget* m_undo_size_limit;
  Widget* m_undo_goto_modified;
  ComboBox* m_screen_scale;
  LinkLabel* m_locate_file;
  ColorButton* m_checked_bg_color1;
  ColorButton* m_checked_bg_color2;
  ColorButton* m_pixelGridColor;
  ColorButton* m_gridColor;
  ColorButton* m_cursorColor;
  Button* m_button_ok;
};

class OptionsCommand : public Command {
public:
  OptionsCommand();
  Command* clone() const OVERRIDE { return new OptionsCommand(*this); }

protected:
  void onExecute(Context* context);
};

OptionsCommand::OptionsCommand()
  : Command("Options",
            "Options",
            CmdUIOnlyFlag)
{
}

void OptionsCommand::onExecute(Context* context)
{
  OptionsWindow window(context);
  window.openWindowInForeground();
  if (window.ok()) 
    window.saveConfig();
}

Command* CommandFactory::createOptionsCommand()
{
  return new OptionsCommand;
}

} // namespace app
