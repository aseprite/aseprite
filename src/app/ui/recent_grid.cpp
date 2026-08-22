// Aseprite
// Copyright (C) 2018-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/file_system.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/thumbnail_generator.h"
#include "app/ui_context.h"
#include "base/fs.h"
#include "skin/skin_theme.h"
#include "ui/alert.h"
#include "ui/animated_widget.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/view.h"

#include "app/ui/recent_grid.h"

#include "app/ui/recent_file.h"

namespace app {

using namespace ui;

constexpr int kThumbnailAnim = 1;

RecentGrid::RecentGrid()
  : Grid(3, false)
  , m_lastColumnCount(0)
  , m_currentThumbnailSize(0)
  , m_mode(Mode::BigThumbnail)
  , m_thumbnailTimer(200, this)
{
  disableFlags(IGNORE_MOUSE);

  m_recentFilesConn = App::instance()->recentFiles()->Changed.connect(
    [this] { rebuildGrid(true); });

  // TODO: We should rebuild the thumbnails after a time and when we change the file.

  m_thumbnailTimer.Tick.connect([this] {
    for (auto* child : children()) {
      auto* recentFile = dynamic_cast<RecentFile*>(child);
      auto* fileItem = FileSystemModule::instance()->getFileItemFromPath(recentFile->path());
      if (fileItem && fileItem->getThumbnailProgress() == 1.0f) {
        recentFile->setThumbnail(fileItem->getThumbnail());
        recentFile->invalidate();
      }
    }

    if (!ThumbnailGenerator::instance()->checkWorkers())
      m_thumbnailTimer.stop();
  });
}

RecentGrid::~RecentGrid()
{
  m_thumbnailTimer.stop();
  ThumbnailGenerator::instance()->stopAllWorkers();
}

void RecentGrid::setMode(const Mode mode)
{
  if (m_mode == mode)
    return;

  m_mode = mode;
  if (m_mode == Mode::List)
    m_rowgap = 4 * guiscale();
  else
    m_rowgap = 16 * guiscale();

  rebuildGrid(true);
}

void RecentGrid::onClick(const std::string& path)
{
  if (!base::is_file(path)) {
    Alert::show(Strings::alerts_recent_file_doesnt_exist());
    App::instance()->recentFiles()->removeRecentFile(path);
    return;
  }

  Command* command = Commands::instance()->byId(CommandId::OpenFile());
  Params params;
  params.set("filename", path.c_str());
  UIContext::instance()->executeCommandFromMenuOrShortcut(command, params);
}

void RecentGrid::onAnimationFrame()
{
  bool change = false;
  for (auto* child : children()) {
    auto* recent = static_cast<RecentFile*>(child);
    if (recent->thumbnailSize() != m_currentThumbnailSize)
      change = true;

    recent->setThumbnailSize(
      inbetween(recent->thumbnailSize(), m_currentThumbnailSize, animationTime()));
  }

  if (change) {
    // Avoid relayouts on startup or when we're not actually moving.
    invalidate();
    View::getView(this)->updateView();
    layout();
  }
}

bool RecentGrid::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kMouseWheelMessage: {
      View::scrollByMessage(this, msg);
      invalidate();
      break;
    }
    case kKeyDownMessage: {
      const auto* keyMsg = static_cast<KeyMessage*>(msg);
      if (m_inline.search(keyMsg)) {
        Widget* focusedChild = manager()->getFocus() && manager()->getFocus()->parent() == this ?
                                 manager()->getFocus() :
                                 firstChild();
        for (Widget* child = focusedChild; child; child = child->nextSibling()) {
          const auto* recent = static_cast<RecentFile*>(child);
          if (m_inline.match(recent->filename())) {
            manager()->setFocus(child);
            return true;
          }
        }
        for (Widget* child = focusedChild; child; child = child->previousSibling()) {
          const auto* recent = static_cast<RecentFile*>(child);
          if (m_inline.match(recent->filename())) {
            manager()->setFocus(child);
            return true;
          }
        }
      }
      break;
    }
    case kMouseDownMessage: {
      const auto* mouseMsg = static_cast<MouseMessage*>(msg);
      if (mouseMsg->right() && mouseMsg->recipient() == this) {
        Menu menu;
        MenuItem clear(Strings::main_menu_file_clear_recent_files());
        clear.processMnemonicFromText();
        menu.addChild(&clear);
        clear.Click.connect([] { App::instance()->recentFiles()->clear(); });
        menu.showPopup(mousePosInDisplay(), display());
        return true;
      }
      break;
    }
    default:;
  }
  return Grid::onProcessMessage(msg);
}

void RecentGrid::onResize(ResizeEvent& ev)
{
  Grid::onResize(ev);
  rebuildGrid();
}

void RecentGrid::onInitTheme(InitThemeEvent& ev)
{
  Grid::onInitTheme(ev);

  setBorder(gfx::Border(4, 0, 4, 4) * guiscale());
  m_colgap = 16 * guiscale();
  if (m_mode == Mode::List)
    m_rowgap = 4 * guiscale();
  else
    m_rowgap = 16 * guiscale();
}

void RecentGrid::addRecent(const std::string& path, const int thumbnailSize, const bool pinned)
{
  auto* fileItem = FileSystemModule::instance()->getFileItemFromPath(path);
  if (!fileItem)
    return;

  if (fileItem->needThumbnail())
    ThumbnailGenerator::instance()->generateThumbnail(fileItem);

  auto* recentFile = new RecentFile(path, thumbnailSize, pinned, m_mode == Mode::List);
  recentFile->setThumbnail(fileItem->getThumbnail());

  addChildInCell(recentFile, 1, 1, NOALIGN);
}

void RecentGrid::rebuildGrid(const bool force)
{
  int width = bounds().w - border().width();
  auto* view = View::getView(this);
  if (view)
    width = view->viewportBounds().w - border().width();

  if (width == 0 && !force)
    return;

  const auto* theme = skin::SkinTheme::get(this);
  int thumbnailSize;
  switch (m_mode) {
    default:
    case Mode::BigThumbnail:   thumbnailSize = theme->dimensions.recentsThumbnailBig(); break;
    case Mode::SmallThumbnail: thumbnailSize = theme->dimensions.recentsThumbnailSmall(); break;
    case Mode::List:           thumbnailSize = theme->dimensions.recentsThumbnailList(); break;
  }
  const int columnCount = m_mode == Mode::List ?
                            1 :
                            std::max<int>(1, std::floor(width / (thumbnailSize + m_colgap)));

  if (m_lastColumnCount == columnCount && !force)
    return;

  std::map<std::string, RecentFile*> prevWidgets;
  int focusIndex = -1;

  for (auto* child : children()) {
    auto* prev = static_cast<RecentFile*>(child);
    if (manager()->getFocus() == prev)
      focusIndex = getChildIndex(prev);
    prevWidgets.try_emplace(prev->path(), prev);
  }
  removeAllChildren();
  clear();

  m_thumbnailTimer.stop();

  const auto* recentFiles = App::instance()->recentFiles();
  const auto& pinned = recentFiles->pinnedFiles();
  const auto& recents = recentFiles->recentFiles();

  setColumns(columnCount);

  for (const auto& pin : pinned) {
    auto it = prevWidgets.find(pin);
    if (it != prevWidgets.end()) {
      it->second->reconfigure(m_mode == Mode::List, true);
      addChildInCell(it->second, 1, 1, NOALIGN);
      prevWidgets.erase(it);
      continue;
    }

    addRecent(pin, thumbnailSize, true);
  }

  for (const auto& recent : recents) {
    if (std::find(pinned.begin(), pinned.end(), recent) == pinned.end()) {
      auto it = prevWidgets.find(recent);
      if (it != prevWidgets.end()) {
        it->second->reconfigure(m_mode == Mode::List, false);
        addChildInCell(it->second, 1, 1, NOALIGN);
        prevWidgets.erase(it);
        continue;
      }

      addRecent(recent, thumbnailSize, false);
    }
  }

  for (const auto& [_, prev] : prevWidgets)
    prev->deferDelete();

  if (focusIndex >= 0) {
    if (children().size() == focusIndex)
      --focusIndex;

    manager()->setFocus(children()[focusIndex]);
  }

  m_thumbnailTimer.start();

  m_currentThumbnailSize = thumbnailSize;
  startAnimation(kThumbnailAnim, 10);

  m_lastColumnCount = columnCount;
  layout();
  if (view)
    view->updateView();
}

} // namespace app
