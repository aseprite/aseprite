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
#include "she/system.h"
#include "ui/ui.h"

#include "generated_options.h"

namespace app {

using namespace ui;

class OptionsWindow : public app::gen::Options {
public:
  OptionsWindow(Context* context)
    : m_settings(context->settings())
    , m_docSettings(m_settings->getDocumentSettings(NULL))
    , m_checked_bg_color1(new ColorButton(RenderEngine::getCheckedBgColor1(), IMAGE_RGB))
    , m_checked_bg_color2(new ColorButton(RenderEngine::getCheckedBgColor2(), IMAGE_RGB))
    , m_pixelGridColor(new ColorButton(m_docSettings->getPixelGridColor(), IMAGE_RGB))
    , m_gridColor(new ColorButton(m_docSettings->getGridColor(), IMAGE_RGB))
    , m_cursorColor(new ColorButton(Editor::get_cursor_color(), IMAGE_RGB))
  {
    sectionListbox()->ChangeSelectedItem.connect(Bind<void>(&OptionsWindow::onChangeSection, this));
    cursorColorBox()->addChild(m_cursorColor);

    // Grid color
    m_gridColor->setId("grid_color");
    gridColorBox()->addChild(m_gridColor);

    // Pixel grid color
    m_pixelGridColor->setId("pixel_grid_color");
    pixelGridColorBox()->addChild(m_pixelGridColor);

    // Others
    if (get_config_bool("Options", "MoveSmooth", true))
      smooth()->setSelected(true);

    if (get_config_bool("Options", "AutoShowTimeline", true))
      autotimeline()->setSelected(true);

    if (m_settings->experimental()->useNativeCursor())
      nativeCursor()->setSelected(true);

    if (m_settings->getShowSpriteEditorScrollbars())
      showScrollbars()->setSelected(true);

    // Checked background size
    screenScale()->addItem("1:1");
    screenScale()->addItem("2:1");
    screenScale()->addItem("3:1");
    screenScale()->addItem("4:1");
    screenScale()->setSelectedItemIndex(get_screen_scaling()-1);

    // Zoom with Scroll Wheel
    wheelZoom()->setSelected(m_settings->getZoomWithScrollWheel());

    // Checked background size
    checkedBgSize()->addItem("16x16");
    checkedBgSize()->addItem("8x8");
    checkedBgSize()->addItem("4x4");
    checkedBgSize()->addItem("2x2");
    checkedBgSize()->setSelectedItemIndex((int)RenderEngine::getCheckedBgType());

    // Zoom checked background
    if (RenderEngine::getCheckedBgZoom())
      checkedBgZoom()->setSelected(true);

    // Checked background colors
    checkedBgColor1Box()->addChild(m_checked_bg_color1);
    checkedBgColor2Box()->addChild(m_checked_bg_color2);

    // Reset button
    checkedBgReset()->Click.connect(Bind<void>(&OptionsWindow::onResetCheckedBg, this));

    // Locate config file
    locateFile()->Click.connect(Bind<void>(&OptionsWindow::onLocateConfigFile, this));

    // Undo limit
    undoSizeLimit()->setTextf("%d", m_settings->undoSizeLimit());

    // Goto modified frame/layer on undo/redo
    if (m_settings->undoGotoModified())
      undoGotoModified()->setSelected(true);

    sectionListbox()->selectIndex(0);
  }

  bool ok() {
    return (getKiller() == buttonOk());
  }

  void saveConfig() {
    Editor::set_cursor_color(m_cursorColor->getColor());
    m_docSettings->setGridColor(m_gridColor->getColor());
    m_docSettings->setPixelGridColor(m_pixelGridColor->getColor());

    set_config_bool("Options", "MoveSmooth", smooth()->isSelected());
    set_config_bool("Options", "AutoShowTimeline", autotimeline()->isSelected());

    m_settings->setShowSpriteEditorScrollbars(showScrollbars()->isSelected());
    m_settings->setZoomWithScrollWheel(wheelZoom()->isSelected());

    RenderEngine::setCheckedBgType((RenderEngine::CheckedBgType)checkedBgSize()->getSelectedItemIndex());
    RenderEngine::setCheckedBgZoom(checkedBgZoom()->isSelected());
    RenderEngine::setCheckedBgColor1(m_checked_bg_color1->getColor());
    RenderEngine::setCheckedBgColor2(m_checked_bg_color2->getColor());

    int undo_size_limit_value;
    undo_size_limit_value = undoSizeLimit()->getTextInt();
    undo_size_limit_value = MID(1, undo_size_limit_value, 9999);

    m_settings->setUndoSizeLimit(undo_size_limit_value);
    m_settings->setUndoGotoModified(undoGotoModified()->isSelected());

    // Native cursor
    m_settings->experimental()->setUseNativeCursor(nativeCursor()->isSelected());

    int new_screen_scaling = screenScale()->getSelectedItemIndex()+1;
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
    ListItem* item = sectionListbox()->getSelectedChild();
    if (!item)
      return;

    panel()->showChild(findChild(item->getValue().c_str()));
  }

  void onResetCheckedBg() {
    // Default values
    checkedBgSize()->setSelectedItemIndex((int)RenderEngine::CHECKED_BG_16X16);
    checkedBgZoom()->setSelected(true);
    m_checked_bg_color1->setColor(app::Color::fromRgb(128, 128, 128));
    m_checked_bg_color2->setColor(app::Color::fromRgb(192, 192, 192));
  }

  void onLocateConfigFile() {
    app::launcher::open_folder(app::get_config_file());
  }

  ISettings* m_settings;
  IDocumentSettings* m_docSettings;
  ColorButton* m_checked_bg_color1;
  ColorButton* m_checked_bg_color2;
  ColorButton* m_pixelGridColor;
  ColorButton* m_gridColor;
  ColorButton* m_cursorColor;
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
