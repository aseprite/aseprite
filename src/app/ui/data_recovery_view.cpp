// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
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
#include "app/closed_docs.h"
#include "app/console.h"
#include "app/crash/data_recovery.h"
#include "app/crash/session.h"
#include "app/doc.h"
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
#include "app/ui_context.h"
#include "base/fs.h"
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

#define DATAVIEW_TRACE(...) // TRACEARGS(__VA_ARGS__)

namespace app {

using namespace ui;
using namespace app::skin;

namespace {

class DataItem : public ListItem {
public:
  DataItem() {}
  DataItem(const std::string& title) : ListItem(title) {}

  virtual bool hasBackup() const { return false; }
  virtual void restoreBackup() {}

protected:
  void onSizeHint(SizeHintEvent& ev) override
  {
    ListItem::onSizeHint(ev);
    gfx::Size sz = ev.sizeHint();
    sz.h += 4 * guiscale();
    ev.setSizeHint(sz);
  }
};

class ClosedDocItem : public DataItem {
public:
  ClosedDocItem(ObjectId id, const std::string& title)
    : DataItem(fmt::format("{} ({})", title, Strings::recover_files_in_memory()))
    , m_id(id)
  {
  }
  bool hasBackup() const override { return m_id != doc::NullId; }
  void restoreBackup() override
  {
    if (auto ctx = UIContext::instance())
      ctx->reopenClosedDocById(m_id);
  }
  ObjectId docId() const { return m_id; }

private:
  ObjectId m_id;
};

class NoClosedDocItem : public DataItem {
public:
  NoClosedDocItem() : DataItem(Strings::recover_files_no_closed_docs()) {}
};

class Item : public DataItem {
public:
  Item(crash::Session* session, const crash::Session::BackupPtr& backup)
    : m_session(session)
    , m_backup(backup)
    , m_task(nullptr)
  {
  }

  crash::Session* session() const { return m_session; }
  const crash::Session::BackupPtr& backup() const { return m_backup; }

  bool isTaskRunning() const { return m_task != nullptr; }

  bool hasBackup() const override { return m_backup != nullptr; }

  void restoreBackup() override
  {
    if (m_task)
      return;
    m_task = new TaskWidget([this](base::task_token& t) {
      // Warning: This is executed from a worker thread
      Doc* doc = m_session->restoreBackupDoc(m_backup, &t);
      ui::execute_from_ui_thread([this, doc] {
        onOpenDoc(doc);
        onDeleteTaskWidget();
      });
    });
    addChild(m_task);
    parent()->layout();
  }

  void restoreRawImages(crash::RawImagesAs as)
  {
    if (m_task)
      return;
    m_task = new TaskWidget([this, as](base::task_token& t) {
      // Warning: This is executed from a worker thread
      Doc* doc = m_session->restoreBackupRawImages(m_backup, as, &t);
      ui::execute_from_ui_thread([this, doc] {
        onOpenDoc(doc);
        onDeleteTaskWidget();
      });
    });
    addChild(m_task);
    updateView();
  }

  void deleteBackup()
  {
    if (m_task)
      return;

    m_task = new TaskWidget(TaskWidget::kCannotCancel, [this](base::task_token& t) {
      try {
        // Warning: This is executed from a worker thread
        m_session->deleteBackup(m_backup);
        m_backup.reset(); // Delete the Backup instance

        ui::execute_from_ui_thread([this] {
          onDeleteTaskWidget();

          // We cannot use this->deferDelete() here because it looks
          // like the m_task field can be still in use.
          setVisible(false);

          updateView();
        });
      }
      catch (const std::exception& ex) {
        std::string err = ex.what();
        if (!err.empty()) {
          ui::execute_from_ui_thread(
            [err] { Console::printf("Error deleting file: %s\n", err.c_str()); });
        }
      }
    });
    addChild(m_task);
    updateView();
  }

  void updateText()
  {
    if (!m_task) {
      ASSERT(m_backup);
      if (!m_backup)
        return;

      setText(m_backup->description(Preferences::instance().general.showFullPath()));
    }
  }

private:
  void onPaint(PaintEvent& ev) override
  {
    // The text is lazily initialized. So we read the backup data only
    // when we have to show its information.
    if (text().empty()) {
      updateText();
    }
    ListItem::onPaint(ev);
  }

  void onResize(ResizeEvent& ev) override
  {
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

  void onOpenDoc(Doc* doc)
  {
    if (!doc)
      return;

    Workspace* workspace = App::instance()->workspace();
    WorkspaceView* restoreView = workspace->activeView();

    // If we have this specific item selected, and the active view in
    // the workspace is the DataRecoveryView, we can open the
    // recovered document. In other case, opening the recovered
    // document is confusing (because the user already selected other
    // item, or other tab from the workspace).
    if (dynamic_cast<DataRecoveryView*>(restoreView) && isSelected()) {
      restoreView = nullptr;
    }

    // Check if the original path of the recovered document exists, in
    // other case remove the path so we use the default one (last path
    // used, or user docs folder, etc.)
    {
      std::string dir = base::get_file_path(doc->filename());
      if (!base::is_directory(dir))
        doc->setFilename(base::get_file_name(doc->filename()));
    }

    UIContext::instance()->documents().add(doc);

    if (restoreView)
      workspace->setActiveView(restoreView);
  }

  void onDeleteTaskWidget()
  {
    if (m_task) {
      removeChild(m_task);
      m_task->deferDelete();
      m_task = nullptr;
      layout();
    }
  }

  void updateView()
  {
    if (auto view = View::getView(this->parent()))
      view->updateView();
    else
      parent()->layout();
  }

  crash::Session* m_session;
  crash::Session::BackupPtr m_backup;
  TaskWidget* m_task;
};

} // anonymous namespace

class DataRecoveryView::SessionSeparator : public SeparatorInView,
                                           public ClosedDocsObserver {
public:
  SessionSeparator(crash::Session* session, bool isRunningSession)
    : SeparatorInView({}, HORIZONTAL)
    , m_session(session)
  {
    std::string title;
    if (isRunningSession) {
      title = Strings::recover_files_active_session();
      m_isRunningSession = true;
    }
    else {
      title = session->name();
      if (session->version() != get_app_version())
        title = Strings::recover_files_incompatible(title, session->version());
      m_isRunningSession = false;
    }

    setText(title);

    if (m_isRunningSession) {
      auto* ctx = UIContext::instance();
      ctx->closedDocs().add_observer(this);
    }
  }

  void onInitTheme(ui::InitThemeEvent& ev) override
  {
    Widget::onInitTheme(ev);
    setBorder(border() + gfx::Border(0, 8, 0, 8) * guiscale());
  }

  ~SessionSeparator()
  {
    if (m_isRunningSession) {
      auto* ctx = UIContext::instance();
      ctx->closedDocs().remove_observer(this);
    }
  }

  void initializeItems(ui::Widget* listBox)
  {
    addClosedDocs(listBox);
    addBackups(listBox);

    if (m_isRunningSession) {
      listBox->addChild(m_noClosedDoc = new NoClosedDocItem);
      m_noClosedDoc->setVisible(m_isEmpty);
    }
  }

  bool isEmpty() const { return m_isEmpty; }

  crash::Session* session() const { return m_session; }

private:
  void addClosedDocs(ui::Widget* listBox)
  {
    if (!m_isRunningSession)
      return;

    const bool fullPath = Preferences::instance().general.showFullPath();
    auto* ctx = UIContext::instance();
    for (const auto& info : ctx->closedDocs().getClosedDocInfos()) {
      auto* item = new ClosedDocItem(info.docId, info.toString(fullPath));
      listBox->addChild(item);
      m_isEmpty = false;
    }
  }

  int clearBackups(ui::Widget* listBox)
  {
    int n = int(parent()->children().size());
    int i = parent()->getChildIndex(this);
    for (++i; i < n; ++i) {
      Widget* child = parent()->at(i);

      // Outside this section
      if (dynamic_cast<Separator*>(child))
        break;

      if (dynamic_cast<Item*>(child)) {
        listBox->removeChild(child);
        child->deferDelete();
        --i;
        --n;
      }
    }
    return i;
  }

  void addBackups(ui::Widget* listBox, int index = -1)
  {
    for (auto& backup : m_session->backups()) {
      auto* item = new Item(m_session, backup);
      if (index < 0)
        listBox->addChild(item);
      else
        listBox->insertChild(index++, item);
      m_isEmpty = false;
    }
  }

  // ClosedDocObserver implementation
  void onNewClosedDoc(const doc::ObjectId docId) override
  {
    DATAVIEW_TRACE("DATAVIEW: onNewClosedDoc", (int)docId);

    int i = parent()->getChildIndex(this);

    const bool fullPath = Preferences::instance().general.showFullPath();
    const crash::DocumentInfo info(doc::get<Doc>(docId));
    parent()->insertChild(i + 1, new ClosedDocItem(info.docId, info.toString(fullPath)));

    updateNoClosedDocVisibility();
  }

  void onReopenClosedDoc(doc::ObjectId docId) override
  {
    DATAVIEW_TRACE("DATAVIEW: onReopenClosedDoc", (int)docId);

    for (auto* widget : parent()->children()) {
      if (auto item = dynamic_cast<ClosedDocItem*>(widget)) {
        if (item->docId() == docId) {
          parent()->removeChild(item);
          item->deferDelete();
          break;
        }
      }
    }

    updateNoClosedDocVisibility();
  }

  void onArchiveClosedDoc(doc::ObjectId docId, const bool backedup) override
  {
    DATAVIEW_TRACE("DATAVIEW: onArchiveClosedDoc", (int)docId, "backedup=", backedup);

    onReopenClosedDoc(docId);

    int index = clearBackups(parent());
    m_session->reloadBackups();
    addBackups(parent(), index);

    updateNoClosedDocVisibility();
  }

  void updateNoClosedDocVisibility()
  {
    ASSERT(m_noClosedDoc);
    if (m_noClosedDoc)
      m_noClosedDoc->setVisible(countItemsInThisSection() == 0);
    parent()->parent()->layout();
  }

  int countItemsInThisSection()
  {
    int itemsInThisSection = 0;
    int n = int(parent()->children().size());
    int i = parent()->getChildIndex(this);
    for (++i; i < n; ++i) {
      Widget* child = parent()->at(i);

      // Outside this section
      if (dynamic_cast<Separator*>(child))
        break;

      // Don't count the "no closed item"
      if (dynamic_cast<NoClosedDocItem*>(child))
        continue;

      ++itemsInThisSection;
    }
    return itemsInThisSection;
  }

  crash::Session* m_session = nullptr;
  NoClosedDocItem* m_noClosedDoc = nullptr;
  bool m_isEmpty = true;
  bool m_isRunningSession = false;
};

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

  InitTheme.connect([this, hbox] {
    auto theme = SkinTheme::get(this);

    m_openButton.mainButton()->resetSizeHint();
    gfx::Size hint = m_openButton.mainButton()->sizeHint();
    m_openButton.mainButton()->setSizeHint(gfx::Size(std::max(hint.w, 100 * guiscale()), hint.h));

    setBgColor(theme->colors.workspace());
    m_view.setStyle(theme->styles.workspaceView());
    hbox->setBorder(gfx::Border(2, 0, 2, 0) * guiscale());
  });
  initTheme();

  fillList();
  onChangeSelection();

  m_openButton.Click.connect([this] { onOpen(); });
  m_openButton.DropDownClick.connect([this] { onOpenMenu(); });
  m_deleteButton.Click.connect([this] { onDelete(); });
  m_refreshButton.Click.connect([this] { onRefresh(); });
  m_listBox.Change.connect([this] { onChangeSelection(); });
  m_listBox.DoubleClickItem.connect([this] { onOpen(); });
  m_waitToEnableRefreshTimer.Tick.connect([this] { onCheckIfWeCanEnableRefreshButton(); });

  m_conn = Preferences::instance().general.showFullPath.AfterChange.connect(
    [this](const bool&) { onShowFullPathPrefChange(); });
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
  for (auto* child : children) {
    m_listBox.removeChild(child);
    child->deferDelete();
  }
  m_runningSession = nullptr;
}

void DataRecoveryView::fillList()
{
  clearList();

  if (m_dataRecovery->isSearching()) {
    m_listBox.addChild(new DataItem(Strings::recover_files_loading()));
    return;
  }

  // Add current session
  addSession(m_dataRecovery->activeSession());

  // Add previous sessions
  for (auto& session : m_dataRecovery->sessions()) {
    if (!session->isEmpty())
      addSession(session.get());
  }
}

void DataRecoveryView::addSession(crash::Session* session)
{
  const bool isRunningSession = m_dataRecovery->isRunningSession(session);

  auto* sep = new SessionSeparator(session, isRunningSession);
  m_listBox.addChild(sep);
  sep->initializeItems(&m_listBox);

  if (isRunningSession)
    m_runningSession = sep;
  else if (sep->isEmpty()) {
    delete sep;
    return;
  }
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

gfx::Color DataRecoveryView::getTabColor()
{
  return gfx::ColorNone;
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

  menu->showPopup(mousePosInDisplay(), display());
}

WidgetsList DataRecoveryView::selectedItems()
{
  WidgetsList selected;
  for (auto widget : m_listBox.children()) {
    if (!widget->isVisible() || !widget->isSelected())
      continue;

    if (auto item = dynamic_cast<DataItem*>(widget))
      selected.push_back(item);
  }
  return selected;
}

void DataRecoveryView::onOpen()
{
  disableRefresh();

  for (auto* widget : selectedItems()) {
    if (auto item = dynamic_cast<DataItem*>(widget)) {
      if (item->hasBackup())
        item->restoreBackup();
    }
  }
}

void DataRecoveryView::onOpenRaw(crash::RawImagesAs as)
{
  disableRefresh();

  for (auto* widget : selectedItems()) {
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

  rawFrames.Click.connect([this] { onOpenRaw(crash::RawImagesAs::kFrames); });
  rawLayers.Click.connect([this] { onOpenRaw(crash::RawImagesAs::kLayers); });

  menu.showPopup(gfx::Point(bounds.x, bounds.y + bounds.h), display());
}

void DataRecoveryView::onDelete()
{
  disableRefresh();

  std::vector<Item*> items; // Items to delete.
  for (auto widget : m_listBox.children()) {
    if (!widget->isVisible() || !widget->isSelected())
      continue;

    if (auto item = dynamic_cast<Item*>(widget)) {
      if (item->backup() && !item->isTaskRunning())
        items.push_back(item);
    }
  }

  if (items.empty())
    return;

  // Delete one backup
  if (Alert::show(Strings::alerts_delete_selected_backups(int(items.size()))) != 1)
    return; // Cancel

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
    if (!widget->isVisible() || !widget->isSelected())
      continue;

    if (auto* item = dynamic_cast<DataItem*>(widget); item && item->hasBackup()) {
      ++count;
    }
  }

  m_deleteButton.setEnabled(count > 0);
  m_openButton.setEnabled(count > 0);
  if (count < 2) {
    m_openButton.mainButton()->setText(Strings::recover_files_recover_sprite());
  }
  else {
    m_openButton.mainButton()->setText(Strings::recover_files_recover_n_sprites(count));
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
      if (item && item->isVisible() && item->session() && item->session()->isCrashedSession())
        return true;
    }
  }
  return false;
}

} // namespace app
