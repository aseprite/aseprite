// Aseprite
// Copyright (C) 2001-2017  David Capello
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
#include "ui/separator.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/view.h"

#include <algorithm>

namespace app {

using namespace ui;
using namespace app::skin;

namespace {

class Item : public ListItem {
public:
  Item(crash::Session* session, crash::Session::Backup* backup)
    : ListItem(backup->description())
    , m_session(session)
    , m_backup(backup)
  {
  }

  crash::Session* session() const { return m_session; }
  crash::Session::Backup* backup() const { return m_backup; }

private:
  void onSizeHint(SizeHintEvent& ev) override {
    ListItem::onSizeHint(ev);
    gfx::Size sz = ev.sizeHint();
    sz.h += 4*guiscale();
    ev.setSizeHint(sz);
  }

  crash::Session* m_session;
  crash::Session::Backup* m_backup;
};

} // anonymous namespace

DataRecoveryView::DataRecoveryView(crash::DataRecovery* dataRecovery)
  : m_dataRecovery(dataRecovery)
  , m_openButton("Recover Sprite")
  , m_deleteButton("Delete")
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  setBgColor(theme->colors.workspace());

  m_openButton.mainButton()->setSizeHint(
    gfx::Size(std::max(m_openButton.mainButton()->sizeHint().w, 100*guiscale()),
              m_openButton.mainButton()->sizeHint().h));

  m_listBox.setMultiselect(true);
  m_view.setExpansive(true);
  m_view.attachToView(&m_listBox);
  m_view.setStyle(theme->newStyles.workspaceView());

  HBox* hbox = new HBox;
  hbox->setBorder(gfx::Border(2, 0, 2, 0)*guiscale());
  hbox->addChild(&m_openButton);
  hbox->addChild(&m_deleteButton);
  addChild(hbox);
  addChild(&m_view);

  fillList();
  onChangeSelection();

  m_openButton.Click.connect(base::Bind(&DataRecoveryView::onOpen, this));
  m_openButton.DropDownClick.connect(base::Bind<void>(&DataRecoveryView::onOpenMenu, this));
  m_deleteButton.Click.connect(base::Bind(&DataRecoveryView::onDelete, this));
  m_listBox.Change.connect(base::Bind(&DataRecoveryView::onChangeSelection, this));
  m_listBox.DoubleClickItem.connect(base::Bind(&DataRecoveryView::onOpen, this));
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

    auto sep = new Separator(session->name(), HORIZONTAL);
    sep->setBgColor(SkinTheme::instance()->colors.background());
    sep->setBorder(sep->border() + gfx::Border(0, 8, 0, 8)*guiscale());
    m_listBox.addChild(sep);

    for (auto& backup : session->backups()) {
      auto item = new Item(session.get(), backup);
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

void DataRecoveryView::onOpen()
{
  for (auto widget : m_listBox.children()) {
    if (!widget->isSelected())
      continue;

    if (auto item = dynamic_cast<Item*>(widget)) {
      if (item->backup())
        item->session()->restoreBackup(item->backup());
    }
  }
}

void DataRecoveryView::onOpenRaw(crash::RawImagesAs as)
{
  for (auto widget : m_listBox.children()) {
    if (!widget->isSelected())
      continue;

    if (auto item = dynamic_cast<Item*>(widget)) {
      if (item->backup())
        item->session()->restoreRawImages(item->backup(), as);
    }
  }
}

void DataRecoveryView::onOpenMenu()
{
  gfx::Rect bounds = m_openButton.bounds();

  Menu menu;
  MenuItem rawFrames("Raw Images as Frames");
  MenuItem rawLayers("Raw Images as Layers");
  menu.addChild(&rawFrames);
  menu.addChild(&rawLayers);

  rawFrames.Click.connect(base::Bind(&DataRecoveryView::onOpenRaw, this, crash::RawImagesAs::kFrames));
  rawLayers.Click.connect(base::Bind(&DataRecoveryView::onOpenRaw, this, crash::RawImagesAs::kLayers));

  menu.showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
}

void DataRecoveryView::onDelete()
{
  std::vector<Item*> items;

  for (auto widget : m_listBox.children()) {
    if (!widget->isSelected())
      continue;

    if (auto item = dynamic_cast<Item*>(widget)) {
      if (item->backup())
        items.push_back(item);
    }
  }

  if (items.empty())
    return;

  // Delete one backup
  if (Alert::show(PACKAGE
                  "<<Do you really want to delete the selected %d backup(s)?"
                  "||&Yes||&No",
                  int(items.size())) != 1)
    return;                     // Cancel

  for (auto item : items) {
    item->session()->deleteBackup(item->backup());
    m_listBox.removeChild(item);
    delete item;
  }
  onChangeSelection();

  m_listBox.layout();
  m_view.updateView();
}

void DataRecoveryView::onChangeSelection()
{
  int count = 0;
  for (auto widget : m_listBox.children()) {
    if (!widget->isSelected())
      continue;

    if (dynamic_cast<Item*>(widget)) {
      ++count;
    }
  }

  m_deleteButton.setEnabled(count > 0);
  m_openButton.setEnabled(count > 0);
  if (count < 2)
    m_openButton.mainButton()->setText("Recover Sprite");
  else
    m_openButton.mainButton()->setTextf("Recover %d Sprites", count);
}

} // namespace app
