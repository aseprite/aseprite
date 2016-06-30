// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/home_view.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/ui/data_recovery_view.h"
#include "app/ui/main_window.h"
#include "app/ui/news_listbox.h"
#include "app/ui/recent_listbox.h"
#include "app/ui/skin/skin_style_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/workspace.h"
#include "app/ui/workspace_tabs.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/exception.h"
#include "ui/label.h"
#include "ui/resize_event.h"
#include "ui/system.h"
#include "ui/textbox.h"
#include "ui/view.h"

namespace app {

using namespace ui;
using namespace app::skin;

HomeView::HomeView()
  : m_files(new RecentFilesListBox)
  , m_folders(new RecentFoldersListBox)
  , m_news(new NewsListBox)
  , m_dataRecovery(nullptr)
  , m_dataRecoveryView(nullptr)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  setBgColor(theme->colors.workspace());
  setChildSpacing(8 * guiscale());

  newFile()->Click.connect(base::Bind(&HomeView::onNewFile, this));
  openFile()->Click.connect(base::Bind(&HomeView::onOpenFile, this));
  recoverSprites()->Click.connect(base::Bind(&HomeView::onRecoverSprites, this));

  filesView()->attachToView(m_files);
  foldersView()->attachToView(m_folders);
  newsView()->attachToView(m_news);

  checkUpdate()->setVisible(false);
  recoverSpritesPlaceholder()->setVisible(false);
}

HomeView::~HomeView()
{
#ifdef ENABLE_DATA_RECOVERY
  if (m_dataRecoveryView) {
    if (m_dataRecoveryView->parent())
      App::instance()->workspace()->removeView(m_dataRecoveryView);
    delete m_dataRecoveryView;
  }
#endif
}

void HomeView::showDataRecovery(crash::DataRecovery* dataRecovery)
{
#ifdef ENABLE_DATA_RECOVERY
  m_dataRecovery = dataRecovery;
  recoverSpritesPlaceholder()->setVisible(true);
#endif
}

std::string HomeView::getTabText()
{
  return "Home";
}

TabIcon HomeView::getTabIcon()
{
  return TabIcon::HOME;
}

bool HomeView::onCloseView(Workspace* workspace, bool quitting)
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
  UIContext::instance()->executeCommand(command);
}

void HomeView::onOpenFile()
{
  Command* command = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
  UIContext::instance()->executeCommand(command);
}

void HomeView::onResize(ui::ResizeEvent& ev)
{
  headerPlaceholder()->setVisible(ev.bounds().h > 200*ui::guiscale());
  foldersPlaceholder()->setVisible(ev.bounds().h > 150*ui::guiscale());
  newsPlaceholder()->setVisible(ev.bounds().w > 200*ui::guiscale());

  ui::VBox::onResize(ev);
}

#ifdef ENABLE_UPDATER

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
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  checkUpdate()->setText("New " PACKAGE " v" + version + " available!");
  checkUpdate()->setUrl(url);
  checkUpdate()->setProperty(
    SkinStylePropertyPtr(new SkinStyleProperty(theme->styles.workspaceUpdateLink())));

  // TODO this should be in a skin.xml's <style>
  gfx::Size iconSize = theme->styles.workspaceUpdateLink()->sizeHint(
    nullptr, Style::State());
  checkUpdate()->setBorder(gfx::Border(6*guiscale())+gfx::Border(
      0, 0, iconSize.w, 0));

  checkUpdate()->setVisible(true);

  layout();
}

#endif // ENABLE_UPDATER

void HomeView::onRecoverSprites()
{
#ifdef ENABLE_DATA_RECOVERY
  if (!m_dataRecoveryView) {
    m_dataRecoveryView = new DataRecoveryView(m_dataRecovery);

    // Hide the "Recover Lost Sprites" button when the
    // DataRecoveryView is empty.
    m_dataRecoveryView->Empty.connect(
      [this]{
        recoverSpritesPlaceholder()->setVisible(false);
        layout();
      });
  }

  if (!m_dataRecoveryView->parent())
    App::instance()->workspace()->addView(m_dataRecoveryView);

  App::instance()->mainWindow()->getTabsBar()->selectTab(m_dataRecoveryView);
#endif
}

} // namespace app
