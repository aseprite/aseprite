// Aseprite
// Copyright (C) 2020-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/extensions.h"
#include "app/modules/gui.h"
#include "app/ui/main_window.h"
#include "app/ui/skin/skin_theme.h"
#include "base/fs.h"
#include "fmt/format.h"
#include "ui/menu.h"
#include "ver/info.h"

#include "zoo.xml.h"

namespace app {

using namespace ui;

namespace {
class ZooWindow final : public gen::Zoo {
public:
  ZooWindow()
  {
    configTabs()->ItemChange.connect([&](ButtonSet::Item* item) {
      switch (configTabs()->getItemIndex(item)) {
        case 0:
          mainContainer()->setEnabled(true);
          changeReadOnly(false);
          break;
        case 1:
          mainContainer()->setEnabled(false);
          changeReadOnly(false);
          break;
        case 2:
          mainContainer()->setEnabled(true);
          changeReadOnly(true);
          break;
      }
      invalidate();
    });

    themeSelector()->Click.connect(&ZooWindow::showThemeMenu, this);

    reset()->Click.connect([this] {
      m_reset = true;
      closeWindow(nullptr);
    });
  }

  void showThemeMenu()
  {
    Menu themeMenu;
    Menu scaleSubMenu;
    auto theme = skin::SkinTheme::get(this);

    for (int i = 1; i <= 4; i++) {
      auto scaleItem = new MenuItem(fmt::format("{}%", i * 100));
      scaleItem->setSelected(guiscale() == i);
      scaleItem->Click.connect([&, i] { ui::set_theme(theme, i); });
      scaleSubMenu.addChild(scaleItem);
    }

    auto* scaleMenu = new MenuItem("UI Scale");
    scaleMenu->setSubmenu(&scaleSubMenu);
    themeMenu.addChild(scaleMenu);

    themeMenu.addChild(new MenuSeparator());

    for (const auto* extension : App::instance()->extensions()) {
      if (!extension->isEnabled())
        continue;

      if (extension->themes().empty())
        continue;

      for (const auto& it : extension->themes()) {
        const std::string id = it.first;

        auto* item = new MenuItem(id);
        item->Click.connect([theme, id] {
          App::instance()->preferences().theme.selected(id);
          ui::set_theme(theme, guiscale());
        });
        item->setSelected(App::instance()->preferences().theme.selected() == id);
        themeMenu.addChild(item);
      }
    }

    themeMenu.showPopup(themeSelector()->bounds().point2() - gfx::Point(themeMenu.sizeHint().w, 0),
                        display());
  }

  void changeReadOnly(bool readOnly)
  {
    entry()->setReadOnly(readOnly);
    entrySuf()->setReadOnly(readOnly);
    entryExpr()->setReadOnly(readOnly);
    entryExprSuf()->setReadOnly(readOnly);
    search()->setReadOnly(readOnly);

    textEdit()->setReadOnly(readOnly);
    slider()->setReadOnly(readOnly);
    sliderAlpha()->setReadOnly(readOnly);
    sliderOpacity()->setReadOnly(readOnly);
    filename()->setReadOnly(readOnly);
    fontPicker()->setReadOnly(readOnly);
  }

  bool shouldReset() const { return m_reset; }

private:
  bool m_reset = false;
};
} // namespace

class WidgetZooCommand : public Command {
public:
  WidgetZooCommand();

protected:
  void onExecute(Context* context) override;
};

WidgetZooCommand::WidgetZooCommand() : Command(CommandId::WidgetZoo())
{
}

void WidgetZooCommand::onExecute(Context* context)
{
  auto startingTheme = App::instance()->preferences().theme.selected();
  auto startingScale = guiscale();

  bool retry;
  do {
    ZooWindow window;
    auto theme = skin::SkinTheme::get(&window);

    window.openWindowInForeground();

    // Reset any themes/scaling to the default when leaving the window.
    if (startingTheme != App::instance()->preferences().theme.selected() ||
        guiscale() != startingScale) {
      App::instance()->preferences().theme.selected(startingTheme);
      ui::set_theme(theme, startingScale);
    }

    // Used when the user pressed the reset button, easier to just re-create everything.
    retry = window.shouldReset();
  } while (retry);
}

Command* CommandFactory::createWidgetZooCommand()
{
  return new WidgetZooCommand;
}

} // namespace app
