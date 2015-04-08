// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/data_recovery_view.h"

#include "app/app_menus.h"
#include "app/crash/data_recovery.h"
#include "app/crash/session.h"
#include "app/modules/gui.h"
#include "app/ui/skin/skin_style_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/workspace.h"
#include "base/bind.h"
#include "ui/button.h"
#include "ui/entry.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/resize_event.h"
#include "ui/system.h"
#include "ui/view.h"

namespace app {

using namespace ui;
using namespace app::skin;

class BackupItem : public ListItem {
public:
  BackupItem(crash::Session* session, crash::Session::Backup* backup)
    : ListItem(" - " + backup->description())
    , m_session(session)
    , m_backup(backup)
    , m_openButton("Open")
    , m_deleteButton("Delete")
    , m_restored(false)
  {
    addChild(&m_openButton);
    addChild(&m_deleteButton);

    m_openButton.Click.connect(Bind(&BackupItem::onOpen, this));
    m_deleteButton.Click.connect(Bind(&BackupItem::onDelete, this));

    setup_mini_look(&m_openButton);
    setup_mini_look(&m_deleteButton);
  }

protected:
  void onResize(ResizeEvent& ev) override {
    ListItem::onResize(ev);

    gfx::Rect rc = ev.getBounds();
    gfx::Size sz1 = m_openButton.getPreferredSize();
    gfx::Size sz2 = m_deleteButton.getPreferredSize();
    m_openButton.setBounds(gfx::Rect(rc.x+rc.w-sz2.w-sz1.w, rc.y, sz1.w, rc.h));
    m_deleteButton.setBounds(gfx::Rect(rc.x+rc.w-sz2.w, rc.y, sz2.w, rc.h));
  }

  void onOpen() {
    if (!m_restored) {
      m_restored = true;
      setText("RESTORED: " + getText());
    }

    m_session->restoreBackup(m_backup);
  }

  void onDelete() {
    m_session->deleteBackup(m_backup);

    getParent()->invalidate();
    deferDelete();
  }

private:
  crash::Session* m_session;
  crash::Session::Backup* m_backup;
  ui::Button m_openButton;
  ui::Button m_deleteButton;
  bool m_restored;
};

DataRecoveryView::DataRecoveryView(crash::DataRecovery* dataRecovery)
  : Box(JI_VERTICAL)
  , m_dataRecovery(dataRecovery)
{
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  setBgColor(theme->colors.workspace());

  addChild(&m_view);
  m_view.setExpansive(true);
  m_view.attachToView(&m_listBox);

  for (auto& session : m_dataRecovery->sessions()) {
    m_listBox.addChild(new ListItem(session->name()));

    for (auto& backup : session->backups())
      m_listBox.addChild(new BackupItem(session.get(), backup));
  }
}

DataRecoveryView::~DataRecoveryView()
{
}

std::string DataRecoveryView::getTabText()
{
  return "Data Recovery";
}

TabIcon DataRecoveryView::getTabIcon()
{
  return TabIcon::NONE;
}

WorkspaceView* DataRecoveryView::cloneWorkspaceView()
{
  return nullptr;               // This view cannot be cloned
}

void DataRecoveryView::onWorkspaceViewSelected()
{
}

void DataRecoveryView::onClonedFrom(WorkspaceView* from)
{
  ASSERT(false);                // Never called
}

bool DataRecoveryView::onCloseView(Workspace* workspace)
{
  workspace->removeView(this);
  return true;
}

void DataRecoveryView::onTabPopup(Workspace* workspace)
{
  Menu* menu = AppMenus::instance()->getTabPopupMenu();
  if (!menu)
    return;

  menu->showPopup(ui::get_mouse_position());
}

} // namespace app
