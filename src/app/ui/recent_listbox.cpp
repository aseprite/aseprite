// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/recent_listbox.h"

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/ui/draggable_widget.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/fs.h"
#include "ui/alert.h"
#include "ui/graphics.h"
#include "ui/link_label.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/scroll_region_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/view.h"

namespace app {

using namespace ui;
using namespace skin;

//////////////////////////////////////////////////////////////////////
// RecentFileItem

class RecentFileItem : public DraggableWidget<LinkLabel> {
public:
  RecentFileItem(const std::string& file,
                 const bool pinned)
    : DraggableWidget<LinkLabel>("")
    , m_fullpath(file)
    , m_name(base::get_file_name(file))
    , m_path(base::get_file_path(file))
    , m_pinned(pinned) {
    initTheme();
  }

  const std::string& fullpath() const { return m_fullpath; }
  bool pinned() const { return m_pinned; }

  void pin() {
    m_pinned = true;
    invalidate();
  }

  void onScrollRegion(ui::ScrollRegionEvent& ev) {
    ev.region() -= gfx::Region(pinBounds(bounds()));
  }

protected:
  void onInitTheme(InitThemeEvent& ev) override {
    LinkLabel::onInitTheme(ev);
    setStyle(SkinTheme::instance()->styles.recentItem());
  }

  void onSizeHint(SizeHintEvent& ev) override {
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    ui::Style* style = theme->styles.recentFile();
    ui::Style* styleDetail = theme->styles.recentFileDetail();

    setTextQuiet(m_name);
    gfx::Size sz1 = theme->calcSizeHint(this, style);

    setTextQuiet(m_path);
    gfx::Size sz2 = theme->calcSizeHint(this, styleDetail);

    ev.setSizeHint(gfx::Size(sz1.w+sz2.w, std::max(sz1.h, sz2.h)));
  }

  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {
      case kMouseDownMessage: {
        const gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        gfx::Rect rc = pinBounds(bounds());
        rc.y = bounds().y;
        rc.h = bounds().h;
        if (rc.contains(mousePos)) {
          m_pinned = !m_pinned;
          invalidate();

          auto parent = this->parent();
          const auto& children = parent->children();
          auto end = children.end();
          auto moveTo = parent->firstChild();
          if (m_pinned) {
            for (auto it=children.begin(); it != end; ++it) {
              if (*it == this || !static_cast<RecentFileItem*>(*it)->pinned()) {
                moveTo = *it;
                break;
              }
            }
          }
          else {
            auto it = std::find(children.begin(), end, this);
            if (it != end) {
              auto prevIt = it++;
              for (; it != end; prevIt=it++) {
                if (!static_cast<RecentFileItem*>(*it)->pinned())
                  break;
              }
              moveTo = *prevIt;
            }
          }
          if (this != moveTo) {
            parent->moveChildTo(this, moveTo);
            parent->layout();
          }
          saveConfig();
          return true;
        }
        break;
      }
    }
    return DraggableWidget<LinkLabel>::onProcessMessage(msg);
  }

  void onPaint(PaintEvent& ev) override {
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    Graphics* g = ev.graphics();
    gfx::Rect bounds = clientBounds();
    ui::Style* style = theme->styles.recentFile();
    ui::Style* styleDetail = theme->styles.recentFileDetail();

    setTextQuiet(m_name.c_str());
    theme->paintWidget(g, this, style, bounds);

    if (Preferences::instance().general.showFullPath()) {
      gfx::Size textSize = theme->calcSizeHint(this, style);
      gfx::Rect detailsBounds(
        bounds.x+textSize.w, bounds.y,
        bounds.w-textSize.w, bounds.h);
      setTextQuiet(m_path.c_str());
      theme->paintWidget(g, this, styleDetail, detailsBounds);
    }

    if (!isDragging() && (m_pinned || hasMouse())) {
      ui::Style* pinStyle = theme->styles.recentFilePin();
      const gfx::Rect pinBounds = this->pinBounds(bounds);
      PaintWidgetPartInfo pi;
      pi.styleFlags =
        (isSelected() ? ui::Style::Layer::kSelected: 0) |
        (m_pinned ? ui::Style::Layer::kFocus: 0) |
        (hasMouse() ? ui::Style::Layer::kMouse: 0);
      theme->paintWidgetPart(g, pinStyle, pinBounds, pi);
    }
  }

  void onClick() override {
    if (!wasDragged())
      static_cast<RecentListBox*>(parent())->onClick(m_fullpath);
  }

  void onReorderWidgets(const gfx::Point& mousePos, bool inside) override {
    auto parent = this->parent();
    auto other = manager()->pick(mousePos);
    if (other && other != this && other->parent() == parent) {
      parent->moveChildTo(this, other);
      parent->layout();
    }
  }

  void onFinalDrop(bool inside) override {
    if (!wasDragged())
      return;

    if (inside) {
      // Pin all elements to keep the order
      const auto& children = parent()->children();
      for (auto it=children.rbegin(), end=children.rend(); it!=end; ++it) {
        if (this == *it) {
          for (; it!=end; ++it)
            static_cast<RecentFileItem*>(*it)->pin();
          break;
        }
      }
    }
    else {
      setVisible(false);
      parent()->layout();
    }

    saveConfig();

    if (!inside)
      deferDelete();
  }

private:
  gfx::Rect pinBounds(const gfx::Rect& bounds) {
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    ui::Style* pinStyle = theme->styles.recentFilePin();
    ui::View* view = View::getView(parent());
    const gfx::Size pinSize = theme->calcSizeHint(this, pinStyle);
    const gfx::Rect vp = view->viewportBounds();
    const gfx::Point scroll = view->viewScroll();
    return gfx::Rect(scroll.x+bounds.x+vp.w-pinSize.w,
                     bounds.y+bounds.h/2-pinSize.h/2,
                     pinSize.w, pinSize.h);
  }

  void saveConfig() {
    static_cast<RecentListBox*>(parent())->updateRecentListFromUIItems();
  }

  std::string m_fullpath;
  std::string m_name;
  std::string m_path;
  bool m_pinned;
};

//////////////////////////////////////////////////////////////////////
// RecentListBox

RecentListBox::RecentListBox()
{
  m_recentFilesConn =
    App::instance()->recentFiles()->Changed.connect(
      base::Bind(&RecentListBox::rebuildList, this));

  m_showFullPathConn =
    Preferences::instance().general.showFullPath.AfterChange.connect(
      base::Bind<void>(&RecentListBox::invalidate, this));
}

void RecentListBox::rebuildList()
{
  while (lastChild()) {
    auto child = lastChild();
    removeChild(child);
    child->deferDelete();
  }

  onRebuildList();

  View* view = View::getView(this);
  if (view)
    view->layout();
  else
    layout();
}

void RecentListBox::updateRecentListFromUIItems()
{
  base::paths pinnedPaths;
  base::paths recentPaths;
  for (auto item : children()) {
    auto fi = static_cast<RecentFileItem*>(item);
    if (fi->hasFlags(ui::HIDDEN))
      continue;
    if (fi->pinned())
      pinnedPaths.push_back(fi->fullpath());
    else
      recentPaths.push_back(fi->fullpath());
  }
  onUpdateRecentListFromUIItems(pinnedPaths,
                                recentPaths);
}

void RecentListBox::onScrollRegion(ui::ScrollRegionEvent& ev)
{
  for (auto item : children())
    static_cast<RecentFileItem*>(item)->onScrollRegion(ev);
}

//////////////////////////////////////////////////////////////////////
// RecentFilesListBox

RecentFilesListBox::RecentFilesListBox()
{
  onRebuildList();
}

void RecentFilesListBox::onRebuildList()
{
  auto recent = App::instance()->recentFiles();
  for (const auto& fn : recent->pinnedFiles())
    addChild(new RecentFileItem(fn, true));
  for (const auto& fn : recent->recentFiles())
    addChild(new RecentFileItem(fn, false));
}

void RecentFilesListBox::onClick(const std::string& path)
{
  if (!base::is_file(path)) {
    ui::Alert::show(Strings::alerts_recent_file_doesnt_exist());
    App::instance()->recentFiles()->removeRecentFile(path);
    return;
  }

  Command* command = Commands::instance()->byId(CommandId::OpenFile());
  Params params;
  params.set("filename", path.c_str());
  UIContext::instance()->executeCommandFromMenuOrShortcut(command, params);
}

void RecentFilesListBox::onUpdateRecentListFromUIItems(const base::paths& pinnedPaths,
                                                       const base::paths& recentPaths)
{
  App::instance()->recentFiles()->setFiles(pinnedPaths,
                                           recentPaths);
}

//////////////////////////////////////////////////////////////////////
// RecentFoldersListBox

RecentFoldersListBox::RecentFoldersListBox()
{
  onRebuildList();
}

void RecentFoldersListBox::onRebuildList()
{
  auto recent = App::instance()->recentFiles();
  for (const auto& fn : recent->pinnedFolders())
    addChild(new RecentFileItem(fn, true));
  for (const auto& fn : recent->recentFolders())
    addChild(new RecentFileItem(fn, false));
}

void RecentFoldersListBox::onClick(const std::string& path)
{
  if (!base::is_directory(path)) {
    ui::Alert::show(Strings::alerts_recent_folder_doesnt_exist());
    App::instance()->recentFiles()->removeRecentFolder(path);
    return;
  }

  Command* command = Commands::instance()->byId(CommandId::OpenFile());
  Params params;
  params.set("folder", path.c_str());
  UIContext::instance()->executeCommandFromMenuOrShortcut(command, params);
}

void RecentFoldersListBox::onUpdateRecentListFromUIItems(const base::paths& pinnedPaths,
                                                         const base::paths& recentPaths)
{
  App::instance()->recentFiles()->setFolders(pinnedPaths,
                                             recentPaths);
}

} // namespace app
