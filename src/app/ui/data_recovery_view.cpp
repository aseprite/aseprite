// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
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
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/ui/drop_down_button.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/workspace.h"
#include "base/bind.h"
#include "fmt/format.h"
#include "ui/alert.h"
#include "ui/button.h"
#include "ui/entry.h"
#include "ui/label.h"
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
  , m_openButton(Strings::recover_files_recover_sprite().c_str())
  , m_deleteButton(Strings::recover_files_delete())
  , m_refreshButton(Strings::recover_files_refresh())
{
  m_listBox.setMultiselect(true);
  m_view.setExpansive(true);
  m_view.attachToView(&m_listBox);

  HBox* hbox = new HBox;
  hbox->addChild(&m_openButton);
  hbox->addChild(&m_deleteButton);
  hbox->addChild(&m_refreshButton);
  addChild(hbox);
  addChild(&m_view);

  InitTheme.connect(
    [this, hbox]{
      SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

      m_openButton.mainButton()->resetSizeHint();
      gfx::Size hint = m_openButton.mainButton()->sizeHint();
      m_openButton.mainButton()->setSizeHint(
        gfx::Size(std::max(hint.w, 100*guiscale()), hint.h));

      setBgColor(theme->colors.workspace());
      m_view.setStyle(theme->styles.workspaceView());
      hbox->setBorder(gfx::Border(2, 0, 2, 0)*guiscale());
    });
  initTheme();

  fillList();
  onChangeSelection();

  m_openButton.Click.connect(base::Bind(&DataRecoveryView::onOpen, this));
  m_openButton.DropDownClick.connect(base::Bind<void>(&DataRecoveryView::onOpenMenu, this));
  m_deleteButton.Click.connect(base::Bind(&DataRecoveryView::onDelete, this));
  m_refreshButton.Click.connect(base::Bind(&DataRecoveryView::onRefresh, this));
  m_listBox.Change.connect(base::Bind(&DataRecoveryView::onChangeSelection, this));
  m_listBox.DoubleClickItem.connect(base::Bind(&DataRecoveryView::onOpen, this));
}

void DataRecoveryView::refreshListNotification()
{
  fillList();
  layout();
}

void DataRecoveryView::clearList()
{
  WidgetsList children = m_listBox.children();
  for (auto child : children) {
    m_listBox.removeChild(child);
    child->deferDelete();
  }
}

void DataRecoveryView::fillList()
{
  clearList();

  if (m_dataRecovery->isSearching())
    m_listBox.addChild(new ListItem(Strings::recover_files_loading()));
  else {
    fillListWith(true);
    fillListWith(false);
  }
}

void DataRecoveryView::fillListWith(const bool crashes)
{
  bool first = true;

  for (auto& session : m_dataRecovery->sessions()) {
    if ((session->isEmpty()) ||
        (crashes && !session->isCrashedSession()) ||
        (!crashes && session->isCrashedSession()))
      continue;

    if (first) {
      first = false;

      // Separator for "crash sessions" vs "old sessions"
      auto sep = new Separator(
        (crashes ? Strings::recover_files_crash_sessions():
                   Strings::recover_files_old_sessions()), HORIZONTAL);
      sep->InitTheme.connect(
        [sep]{
          sep->setStyle(skin::SkinTheme::instance()->styles.separatorInViewReverse());
          sep->setBorder(sep->border() + gfx::Border(0, 8, 0, 8)*guiscale());
        });
      sep->initTheme();
      m_listBox.addChild(sep);
    }

    std::string title = session->name();
    if (session->version() != VERSION)
      title =
        fmt::format(Strings::recover_files_incompatible(),
                    title, session->version());

    auto sep = new SeparatorInView(title, HORIZONTAL);
    sep->InitTheme.connect(
      [sep]{
        sep->setBorder(sep->border() + gfx::Border(0, 8, 0, 8)*guiscale());
      });
    sep->initTheme();
    m_listBox.addChild(sep);

    for (auto& backup : session->backups()) {
      auto item = new Item(session.get(), backup);
      m_listBox.addChild(item);
    }
  }

  // If there are no crash items, we call Empty() signal
  if (crashes && first)
    Empty();
}

std::string DataRecoveryView::getTabText()
{
  return Strings::recover_files_title();
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
  MenuItem rawFrames(Strings::recover_files_raw_images_as_frames());
  MenuItem rawLayers(Strings::recover_files_raw_images_as_layers());
  menu.addChild(&rawFrames);
  menu.addChild(&rawLayers);

  rawFrames.Click.connect(base::Bind(&DataRecoveryView::onOpenRaw, this, crash::RawImagesAs::kFrames));
  rawLayers.Click.connect(base::Bind(&DataRecoveryView::onOpenRaw, this, crash::RawImagesAs::kLayers));

  menu.showPopup(gfx::Point(bounds.x, bounds.y+bounds.h));
}

void DataRecoveryView::onDelete()
{
  std::vector<Item*> items; // Items to delete.

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
  if (Alert::show(
        fmt::format(Strings::alerts_delete_selected_backups(),
                    int(items.size()))) != 1)
    return;                     // Cancel

  for (auto item : items) {
    item->session()->deleteBackup(item->backup());
    m_listBox.removeChild(item);
    delete item;
  }
  onChangeSelection();

  // Check if there is no more crash sessions
  if (!thereAreCrashSessions())
    Empty();

  m_listBox.layout();
  m_view.updateView();
}

void DataRecoveryView::onRefresh()
{
  m_dataRecovery->launchSearch();

  fillList();
  onChangeSelection();
  layout();
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
  if (count < 2) {
    m_openButton.mainButton()->setText(
      fmt::format(Strings::recover_files_recover_sprite(), count));
  }
  else {
    m_openButton.mainButton()->setText(
      fmt::format(Strings::recover_files_recover_n_sprites(), count));
  }
}

bool DataRecoveryView::thereAreCrashSessions() const
{
  for (auto widget : m_listBox.children()) {
    if (auto item = dynamic_cast<const Item*>(widget)) {
      if (item &&
          item->session() &&
          item->session()->isCrashedSession())
        return true;
    }
  }
  return false;
}

} // namespace app
