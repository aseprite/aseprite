// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "app/send_crash.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/ui/color_button.h"
#include "app/ui/editor/editor.h"
#include "base/bind.h"
#include "base/path.h"
#include "doc/image.h"
#include "render/render.h"
#include "she/system.h"
#include "ui/ui.h"

#include "generated_options.h"

namespace app {

using namespace ui;

class OptionsWindow : public app::gen::Options {
public:
  OptionsWindow(Context* context)
    : m_settings(context->settings())
    , m_docSettings(m_settings->getDocumentSettings(context->activeDocument()))
    , m_preferences(App::instance()->preferences())
    , m_docPref(m_preferences.document(context->activeDocument()))
    , m_checked_bg_color1(new ColorButton(m_docPref.bg.color1(), IMAGE_RGB))
    , m_checked_bg_color2(new ColorButton(m_docPref.bg.color2(), IMAGE_RGB))
    , m_pixelGridColor(new ColorButton(m_docSettings->getPixelGridColor(), IMAGE_RGB))
    , m_gridColor(new ColorButton(m_docSettings->getGridColor(), IMAGE_RGB))
    , m_cursorColor(new ColorButton(Editor::get_cursor_color(), IMAGE_RGB))
  {
    sectionListbox()->ChangeSelectedItem.connect(Bind<void>(&OptionsWindow::onChangeSection, this));
    cursorColorBox()->addChild(m_cursorColor);

    // Grid color
    m_gridColor->setId("grid_color");
    gridColorPlaceholder()->addChild(m_gridColor);
    gridOpacity()->setValue(m_docSettings->getGridOpacity());
    gridAutoOpacity()->setSelected(m_docSettings->getGridAutoOpacity());

    // Pixel grid color
    m_pixelGridColor->setId("pixel_grid_color");
    pixelGridColorPlaceholder()->addChild(m_pixelGridColor);
    pixelGridOpacity()->setValue(m_docSettings->getPixelGridOpacity());
    pixelGridAutoOpacity()->setSelected(m_docSettings->getPixelGridAutoOpacity());

    // Others
    if (m_preferences.general.autoshowTimeline())
      autotimeline()->setSelected(true);

    if (m_preferences.general.expandMenubarOnMouseover())
      expandMenubarOnMouseover()->setSelected(true);

    if (m_settings->getCenterOnZoom())
      centerOnZoom()->setSelected(true);

    if (m_settings->experimental()->useNativeCursor())
      nativeCursor()->setSelected(true);

    if (m_settings->experimental()->flashLayer())
      flashLayer()->setSelected(true);

    if (m_settings->getShowSpriteEditorScrollbars())
      showScrollbars()->setSelected(true);

    // Checked background size
    screenScale()->addItem("1:1");
    screenScale()->addItem("2:1");
    screenScale()->addItem("3:1");
    screenScale()->addItem("4:1");
    screenScale()->setSelectedItemIndex(get_screen_scaling()-1);

    // Right-click
    rightClickBehavior()->addItem("Paint with background color");
    rightClickBehavior()->addItem("Pick foreground color");
    rightClickBehavior()->addItem("Erase");
    rightClickBehavior()->setSelectedItemIndex((int)m_settings->getRightClickMode());

    // Zoom with Scroll Wheel
    wheelZoom()->setSelected(m_settings->getZoomWithScrollWheel());

    // Checked background size
    checkedBgSize()->addItem("16x16");
    checkedBgSize()->addItem("8x8");
    checkedBgSize()->addItem("4x4");
    checkedBgSize()->addItem("2x2");
    checkedBgSize()->setSelectedItemIndex(int(m_docPref.bg.type()));

    // Zoom checked background
    if (m_docPref.bg.zoom())
      checkedBgZoom()->setSelected(true);

    // Checked background colors
    checkedBgColor1Box()->addChild(m_checked_bg_color1);
    checkedBgColor2Box()->addChild(m_checked_bg_color2);

    // Reset button
    reset()->Click.connect(Bind<void>(&OptionsWindow::onReset, this));

    // Links
    locateFile()->Click.connect(Bind<void>(&OptionsWindow::onLocateConfigFile, this));
#if WIN32
    locateCrashFolder()->Click.connect(Bind<void>(&OptionsWindow::onLocateCrashFolder, this));
#else
    locateCrashFolder()->setVisible(false);
#endif

    // Undo preferences
    undoSizeLimit()->setTextf("%d", m_preferences.undo.sizeLimit());
    undoGotoModified()->setSelected(m_preferences.undo.gotoModified());
    undoAllowNonlinearHistory()->setSelected(m_preferences.undo.allowNonlinearHistory());

    sectionListbox()->selectIndex(0);
  }

  bool ok() {
    return (getKiller() == buttonOk());
  }

  void saveConfig() {
    Editor::set_cursor_color(m_cursorColor->getColor());
    m_docSettings->setGridColor(m_gridColor->getColor());
    m_docSettings->setGridOpacity(gridOpacity()->getValue());
    m_docSettings->setGridAutoOpacity(gridAutoOpacity()->isSelected());
    m_docSettings->setPixelGridColor(m_pixelGridColor->getColor());
    m_docSettings->setPixelGridOpacity(pixelGridOpacity()->getValue());
    m_docSettings->setPixelGridAutoOpacity(pixelGridAutoOpacity()->isSelected());

    m_preferences.general.autoshowTimeline(autotimeline()->isSelected());

    bool expandOnMouseover = expandMenubarOnMouseover()->isSelected();
    m_preferences.general.expandMenubarOnMouseover(expandOnMouseover);
    ui::MenuBar::setExpandOnMouseover(expandOnMouseover);

    m_settings->setCenterOnZoom(centerOnZoom()->isSelected());

    m_settings->setShowSpriteEditorScrollbars(showScrollbars()->isSelected());
    m_settings->setZoomWithScrollWheel(wheelZoom()->isSelected());
    m_settings->setRightClickMode(static_cast<RightClickMode>(rightClickBehavior()->getSelectedItemIndex()));

    m_docPref.bg.type(app::gen::BgType(checkedBgSize()->getSelectedItemIndex()));
    m_docPref.bg.zoom(checkedBgZoom()->isSelected());
    m_docPref.bg.color1(m_checked_bg_color1->getColor());
    m_docPref.bg.color2(m_checked_bg_color2->getColor());

    int undo_size_limit_value;
    undo_size_limit_value = undoSizeLimit()->getTextInt();
    undo_size_limit_value = MID(1, undo_size_limit_value, 9999);

    m_preferences.undo.sizeLimit(undo_size_limit_value);
    m_preferences.undo.gotoModified(undoGotoModified()->isSelected());
    m_preferences.undo.allowNonlinearHistory(undoAllowNonlinearHistory()->isSelected());

    // Experimental features
    m_settings->experimental()->setUseNativeCursor(nativeCursor()->isSelected());
    m_settings->experimental()->setFlashLayer(flashLayer()->isSelected());

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

  void onReset() {
    // Default values
    // TODO improve settings and default values (store everything in
    // an XML and generate code from it)

    m_gridColor->setColor(app::Color::fromRgb(0, 0, 255));
    gridOpacity()->setValue(200);
    gridAutoOpacity()->setSelected(true);

    m_pixelGridColor->setColor(app::Color::fromRgb(200, 200, 200));
    pixelGridOpacity()->setValue(200);
    pixelGridAutoOpacity()->setSelected(true);

    checkedBgSize()->setSelectedItemIndex(int(m_docPref.bg.type.defaultValue()));
    checkedBgZoom()->setSelected(m_docPref.bg.zoom.defaultValue());
    m_checked_bg_color1->setColor(m_docPref.bg.color1.defaultValue());
    m_checked_bg_color2->setColor(m_docPref.bg.color2.defaultValue());
  }

  void onLocateCrashFolder() {
    app::launcher::open_folder(base::get_file_path(app::memory_dump_filename()));
  }

  void onLocateConfigFile() {
    app::launcher::open_folder(app::main_config_filename());
  }

  ISettings* m_settings;
  IDocumentSettings* m_docSettings;
  Preferences& m_preferences;
  DocumentPreferences& m_docPref;
  ColorButton* m_checked_bg_color1;
  ColorButton* m_checked_bg_color2;
  ColorButton* m_pixelGridColor;
  ColorButton* m_gridColor;
  ColorButton* m_cursorColor;
};

class OptionsCommand : public Command {
public:
  OptionsCommand();
  Command* clone() const override { return new OptionsCommand(*this); }

protected:
  void onExecute(Context* context);
};

OptionsCommand::OptionsCommand()
  : Command("Options",
            "Options",
            CmdUIOnlyFlag)
{
  Preferences& preferences = App::instance()->preferences();

  ui::MenuBar::setExpandOnMouseover(
    preferences.general.expandMenubarOnMouseover());
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
