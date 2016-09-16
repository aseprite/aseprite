// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/data_recovery_view.h"

#include "app/app_menus.h"
#include "app/crash/data_recovery.h"
#include "app/crash/session.h"
#include "app/modules/gui.h"
#include "app/ui/drop_down_button.h"
#include "app/ui/skin/skin_style_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/workspace.h"
#include "base/bind.h"
#include "ui/alert.h"
#include "ui/button.h"
#include "ui/entry.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/resize_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/view.h"

namespace app {

using namespace ui;
using namespace app::skin;

namespace {

class Item : public ListItem {
public:
  Item(crash::Session* session, crash::Session::Backup* backup)
    : ListItem(backup ? " > " + backup->description(): session->name())
    , m_session(session)
    , m_backup(backup)
    , m_openButton(backup ? "Open": "Open All")
    , m_deleteButton(backup ? "Delete": "Delete All")
  {
    m_hbox.setBgColor(gfx::ColorNone);
    m_hbox.setTransparent(true);
    m_hbox.addChild(&m_openButton);
    m_hbox.addChild(&m_deleteButton);
    addChild(&m_hbox);

    m_openButton.Click.connect(base::Bind(&Item::onOpen, this));
    m_openButton.DropDownClick.connect(base::Bind<void>(&Item::onOpenMenu, this));
    m_deleteButton.Click.connect(base::Bind(&Item::onDelete, this));

    setup_mini_look(&m_openButton);
    setup_mini_look(&m_deleteButton);
  }

  obs::signal<void()> Regenerate;

protected:
  void onSizeHint(SizeHintEvent& ev) override {
    gfx::Size sz = m_deleteButton.sizeHint();
    sz.h += 4*guiscale();
    ev.setSizeHint(sz);
  }

  void onResize(ResizeEvent& ev) override {
    ListItem::onResize(ev);

    gfx::Rect rc = ev.bounds();
    gfx::Size sz = m_hbox.sizeHint();
    m_hbox.setBounds(
      gfx::Rect(
        rc.x+rc.w-sz.w-2*guiscale(), rc.y+rc.h/2-sz.h/2, sz.w, sz.h));
  }

  void onOpen() {
    if (m_backup)
      m_session->restoreBackup(m_backup);
    else
      for (auto backup : m_session->backups())
        m_session->restoreBackup(backup);
  }

  void onOpenRaw(crash::RawImagesAs as) {
    if (m_backup)
      m_session->restoreRawImages(m_backup, as);
    else
      for (auto backup : m_session->backups())
        m_session->restoreRawImages(backup, as);
  }

  void onOpenMenu() {
    gfx::Rect bounds = m_openButton.bounds();

    Menu menu;
    MenuItem rawFrames("Raw Images as Frames");
    MenuItem rawLayers("Raw Images as Layers");
    menu.addChild(&rawFrames);
    menu.addChild(&rawLayers);

    rawFrames.Click.connect(base::Bind(&Item::onOpenRaw, this, crash::RawImagesAs::kFrames));
    rawLayers.Click.connect(base::Bind(&Item::onOpenRaw, this, crash::RawImagesAs::kLayers));

    menu.showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
  }

  void onDelete() {
    Widget* parent = this->parent();

    if (m_backup) {
      // Delete one backup
      if (Alert::show(PACKAGE
          "<<Do you really want to delete this backup?"
          "||&Yes||&No") != 1)
        return;

      m_session->deleteBackup(m_backup);

      Widget* parent = this->parent(); // TODO remove this line
      parent->removeChild(this);
      deferDelete();
    }
    else {
      // Delete the whole session
      if (!m_session->isEmpty()) {
        if (Alert::show(PACKAGE
            "<<Do you want to delete the whole session?"
            "<<You will lost all backups related to this session."
            "||&Yes||&No") != 1)
          return;
      }

      crash::Session::Backups backups = m_session->backups();
      for (auto backup : backups)
        m_session->deleteBackup(backup);

      m_session->removeFromDisk();
      Regenerate();
    }

    parent->layout();
    View::getView(parent)->updateView();
  }

private:
  crash::Session* m_session;
  crash::Session::Backup* m_backup;
  ui::HBox m_hbox;
  DropDownButton m_openButton;
  ui::Button m_deleteButton;
};

} // anonymous namespace

DataRecoveryView::DataRecoveryView(crash::DataRecovery* dataRecovery)
  : Box(VERTICAL)
  , m_dataRecovery(dataRecovery)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  setBgColor(theme->colors.workspace());

  addChild(&m_view);
  m_view.setExpansive(true);
  m_view.attachToView(&m_listBox);
  m_view.setProperty(SkinStylePropertyPtr(new SkinStyleProperty(theme->styles.workspaceView())));

  fillList();
}

DataRecoveryView::~DataRecoveryView()
{
}

void DataRecoveryView::fillList()
{
  WidgetsList children = m_listBox.children();
  for (auto child : children) {
    m_listBox.removeChild(child);
    child->deferDelete();
  }

  for (auto& session : m_dataRecovery->sessions()) {
    if (session->isEmpty())
      continue;

    Item* item = new Item(session.get(), nullptr);
    item->Regenerate.connect(&DataRecoveryView::fillList, this);
    m_listBox.addChild(item);

    for (auto& backup : session->backups()) {
      item = new Item(session.get(), backup);
      item->Regenerate.connect(&DataRecoveryView::fillList, this);
      m_listBox.addChild(item);
    }
  }

  if (m_listBox.getItemsCount() == 0)
    Empty();
}

std::string DataRecoveryView::getTabText()
{
  return "Data Recovery";
}

TabIcon DataRecoveryView::getTabIcon()
{
  return TabIcon::NONE;
}

void DataRecoveryView::onWorkspaceViewSelected()
{
}

bool DataRecoveryView::onCloseView(Workspace* workspace, bool quitting)
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
