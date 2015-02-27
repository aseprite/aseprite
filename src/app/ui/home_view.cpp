// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/home_view.h"

#include "app/app_menus.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/ui/skin/skin_style_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/exception.h"
#include "ui/label.h"
#include "ui/system.h"
#include "ui/textbox.h"
#include "ui/view.h"

namespace app {

using namespace ui;
using namespace app::skin;

HomeView::HomeView()
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  setBgColor(theme->colors.workspace());

  child_spacing = 8 * guiscale();

  newFile()->Click.connect(Bind(&HomeView::onNewFile, this));
  openFile()->Click.connect(Bind(&HomeView::onOpenFile, this));

  checkUpdate()->setVisible(false);
}

HomeView::~HomeView()
{
}

std::string HomeView::getTabText()
{
  return "Home";
}

TabIcon HomeView::getTabIcon()
{
  return TabIcon::HOME;
}

WorkspaceView* HomeView::cloneWorkspaceView()
{
  return NULL;                  // This view cannot be cloned
}

void HomeView::onClonedFrom(WorkspaceView* from)
{
  ASSERT(false);                // Never called
}

bool HomeView::onCloseView(Workspace* workspace)
{
  workspace->removeView(this);
  return true;
}

void HomeView::onTabPopup(Workspace* workspace)
{
  Menu* menu = AppMenus::instance()->getTabPopupMenu();
  if (!menu)
    return;

  menu->showPopup(ui::get_mouse_position());
}

void HomeView::onWorkspaceViewSelected()
{
}

void HomeView::onNewFile()
{
  Command* command = CommandsModule::instance()->getCommandByName(CommandId::NewFile);
  Params params;
  UIContext::instance()->executeCommand(command, &params);
}

void HomeView::onOpenFile()
{
  Command* command = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
  Params params;
  UIContext::instance()->executeCommand(command, &params);
}

void HomeView::onCheckingUpdates()
{
  checkUpdate()->setText("Checking Updates...");
  checkUpdate()->setVisible(true);

  layout();
}

void HomeView::onUpToDate()
{
  checkUpdate()->setText(PACKAGE " is up to date");
  checkUpdate()->setVisible(true);

  layout();
}

void HomeView::onNewUpdate(const std::string& url, const std::string& version)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());

  checkUpdate()->setText("New " PACKAGE " v" + version + " available!");
  checkUpdate()->setUrl(url);
  checkUpdate()->setProperty(
    SkinStylePropertyPtr(new SkinStyleProperty(theme->styles.workspaceUpdateLink())));

  // TODO this should be in a skin.xml's <style>
  gfx::Size iconSize = theme->styles.workspaceUpdateLink()->preferredSize(
    nullptr, Style::State());
  checkUpdate()->setBorder(gfx::Border(6*guiscale())+gfx::Border(
      0, 0, iconSize.w, 0));

  checkUpdate()->setVisible(true);

  layout();
}

} // namespace app
