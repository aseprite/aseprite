// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/data_recovery_view.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/crash/data_recovery.h"
#include "app/crash/session.h"
#include "app/i18n/strings.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/task.h"
#include "app/ui/data_recovery_view.h"
#include "app/ui/drop_down_button.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/task_widget.h"
#include "app/ui/workspace.h"
#include "app/ui/workspace.h"
#include "app/ui_context.h"
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
#include "ver/info.h"

#include <algorithm>

namespace app {

using namespace ui;
using namespace app::skin;

namespace {

class Item : public ListItem {
public:
  Item(crash::Session* session, crash::Session::Backup* backup)
    : m_session(session)
    , m_backup(backup)
    , m_task(nullptr) {
    updateText();
  }

  crash::Session* session() const { return m_session; }
  crash::Session::Backup* backup() const { return m_backup; }

  bool isTaskRunning() const { return m_task != nullptr; }

  void restoreBackup() {
    if (m_task)
      return;
    m_task = new TaskWidget(
      [this](base::task_token& t) {
        // Warning: This is executed from a worker thread
        Doc* doc = m_session->restoreBackupDoc(m_backup, &t);
        ui::execute_from_ui_thread(
          [this, doc]{
            onOpenDoc(doc);
            onDeleteTaskWidget();
          });
      });
    addChild(m_task);
    parent()->layout();
  }

  void restoreRawImages(crash::RawImagesAs as) {
    if (m_task)
      return;
    m_task = new TaskWidget(
      [this, as](base::task_token& t){
        // Warning: This is executed from a worker thread
        Doc* doc = m_session->restoreBackupRawImages(m_backup, as, &t);
        ui::execute_from_ui_thread(
          [this, doc]{
            onOpenDoc(doc);
            onDeleteTaskWidget();
          });
      });
    addChild(m_task);
    updateView();
  }

  void deleteBackup() {
    if (m_task)
      return;

    m_task = new TaskWidget(
      TaskWidget::kCannotCancel,
      [this](base::task_token& t){
        // Warning: This is executed from a worker thread
        m_session->deleteBackup(m_backup);
        ui::execute_from_ui_thread(
          [this]{
            onDeleteTaskWidget();

            // We cannot use this->deferDelete() here because it looks
            // like the m_task field can be still in use.
            setVisible(false);

            updateView();
          });
      });
    addChild(m_task);
    updateView();
  }

  void updateText() {
    if (!m_task)
      setText(
        m_backup->description(
          Preferences::instance().general.showFullPath()));
  }

private:
  void onSizeHint(SizeHintEvent& ev) override {
    ListItem::onSizeHint(ev);
    gfx::Size sz = ev.sizeHint();
    sz.h += 4*guiscale();
    ev.setSizeHint(sz);
  }

  void onResize(ResizeEvent& ev) override {
    setBoundsQuietly(ev.bounds());

    if (m_task) {
      gfx::Size sz = m_task->sizeHint();
      gfx::Rect cpos = childrenBounds();

      int x2;
      if (auto view = View::getView(this->parent()))
        x2 = view->viewportBounds().x2();
      else
        x2 = cpos.x2();

      cpos.x = x2 - sz.w;
      cpos.w = sz.w;
      m_task->setBounds(cpos);
    }
  }

  void onOpenDoc(Doc* doc) {
    if (!doc)
      return;

    Workspace* workspace = App::instance()->workspace();
    WorkspaceView* restoreView = workspace->activeView();

    // If we have this specific item selected, and the active view in
    // the workspace is the DataRecoveryView, we can open the
    // recovered document. In other case, opening the recovered
    // document is confusing (because the user already selected other
    // item, or other tab from the workspace).
    if (dynamic_cast<DataRecoveryView*>(restoreView) &&
        isSelected()) {
      restoreView = nullptr;
    }

    UIContext::instance()->documents().add(doc);

    if (restoreView)
      workspace->setActiveView(restoreView);
  }

  void onDeleteTaskWidget() {
    if (m_task) {
      removeChild(m_task);
      m_task->deferDelete();
      m_task = nullptr;
      layout();
    }
  }

  void updateView() {
    if (auto view = View::getView(this->parent()))
      view->updateView();
    else
      parent()->layout();
  }

  crash::Session* m_session;
  crash::Session::Backup* m_backup;
  TaskWidget* m_task;
};

} // anonymous namespace

DataRecoveryView::DataRecoveryView(crash::DataRecovery* dataRecovery)
  : m_dataRecovery(dataRecovery)
  , m_openButton(Strings::recover_files_recover_sprite().c_str())
  , m_deleteButton(Strings::recover_files_delete())
  , m_refreshButton(Strings::recover_files_refresh())
  , m_waitToEnableRefreshTimer(100)
{
  m_listBox.setMultiselect(true);
  m_view.setExpansive(true);
  m_view.attachToView(&m_listBox);

  HBox* hbox = new HBox;
  hbox->addChild(&m_openButton);
  hbox->addChild(&m_refreshButton);
  hbox->addChild(new BoxFiller);
  hbox->addChild(&m_deleteButton);
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
  m_waitToEnableRefreshTimer.Tick.connect(base::Bind(&DataRecoveryView::onCheckIfWeCanEnableRefreshButton, this));

  m_conn = Preferences::instance()
    .general.showFullPath.AfterChange.connect(
      [this](const bool&){ onShowFullPathPrefChange(); });
}

DataRecoveryView::~DataRecoveryView()
{
  m_conn.disconnect();
}

void DataRecoveryView::refreshListNotification()
{
  if (someItemIsBusy())
    return;

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
    if (session->version() != get_app_version())
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

void DataRecoveryView::disableRefresh()
{
  m_refreshButton.setEnabled(false);
  m_waitToEnableRefreshTimer.start();
}

bool DataRecoveryView::someItemIsBusy()
{
  // Just in case check that we are not already running some task (so
  // we cannot refresh the list)
  for (auto widget : m_listBox.children()) {
    if (auto item = dynamic_cast<Item*>(widget)) {
      if (item->isTaskRunning())
        return true;
    }
  }
  return false;
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
  // Do nothing
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
  disableRefresh();

  for (auto widget : m_listBox.children()) {
    if (!widget->isVisible() ||
        !widget->isSelected())
      continue;

    if (auto item = dynamic_cast<Item*>(widget)) {
      if (item->backup())
        item->restoreBackup();
    }
  }
}

void DataRecoveryView::onOpenRaw(crash::RawImagesAs as)
{
  disableRefresh();

  for (auto widget : m_listBox.children()) {
    if (!widget->isVisible() ||
        !widget->isSelected())
      continue;

    if (auto item = dynamic_cast<Item*>(widget)) {
      if (item->backup())
        item->restoreRawImages(as);
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
  disableRefresh();

  std::vector<Item*> items; // Items to delete.
  for (auto widget : m_listBox.children()) {
    if (!widget->isVisible() ||
        !widget->isSelected())
      continue;

    if (auto item = dynamic_cast<Item*>(widget)) {
      if (item->backup() &&
          !item->isTaskRunning())
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

  for (auto item : items)
    item->deleteBackup();
}

void DataRecoveryView::onRefresh()
{
  if (someItemIsBusy())
    return;

  m_dataRecovery->launchSearch();

  fillList();
  onChangeSelection();
  layout();
}

void DataRecoveryView::onChangeSelection()
{
  int count = 0;
  for (auto widget : m_listBox.children()) {
    if (!widget->isVisible() ||
        !widget->isSelected())
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

void DataRecoveryView::onCheckIfWeCanEnableRefreshButton()
{
  if (someItemIsBusy())
    return;

  m_refreshButton.setEnabled(true);
  m_waitToEnableRefreshTimer.stop();

  onChangeSelection();

  // Check if there is no more crash sessions
  if (!thereAreCrashSessions())
    Empty();

  m_listBox.layout();
  m_view.updateView();
}

void DataRecoveryView::onShowFullPathPrefChange()
{
  for (auto widget : m_listBox.children()) {
    if (auto item = dynamic_cast<Item*>(widget)) {
      if (!item->isTaskRunning())
        item->updateText();
    }
  }
}

bool DataRecoveryView::thereAreCrashSessions() const
{
  for (auto widget : m_listBox.children()) {
    if (auto item = dynamic_cast<const Item*>(widget)) {
      if (item &&
          item->isVisible() &&
          item->session() &&
          item->session()->isCrashedSession())
        return true;
    }
  }
  return false;
}

} // namespace app
