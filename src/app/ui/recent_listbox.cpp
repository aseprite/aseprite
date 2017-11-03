// Aseprite
// Copyright (C) 2001-2017  David Capello
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
#include "app/pref/preferences.h"
#include "app/recent_files.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "base/fs.h"
#include "ui/graphics.h"
#include "ui/link_label.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/view.h"

namespace app {

using namespace ui;
using namespace skin;

//////////////////////////////////////////////////////////////////////
// RecentFileItem

class RecentFileItem : public LinkLabel {
public:
  RecentFileItem(const std::string& file)
    : LinkLabel("")
    , m_fullpath(file)
    , m_name(base::get_file_name(file))
    , m_path(base::get_file_path(file)) {
    initTheme();
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

    ev.setSizeHint(gfx::Size(sz1.w+sz2.w, MAX(sz1.h, sz2.h)));
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
  }

  void onClick() override {
    static_cast<RecentListBox*>(parent())->onClick(m_fullpath);
  }

private:
  std::string m_fullpath;
  std::string m_name;
  std::string m_path;
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

//////////////////////////////////////////////////////////////////////
// RecentFilesListBox

RecentFilesListBox::RecentFilesListBox()
{
  onRebuildList();
}

void RecentFilesListBox::onRebuildList()
{
  auto recent = App::instance()->recentFiles();
  auto it = recent->files_begin();
  auto end = recent->files_end();
  for (; it != end; ++it)
    addChild(new RecentFileItem(it->c_str()));
}

void RecentFilesListBox::onClick(const std::string& path)
{
  Command* command = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
  Params params;
  params.set("filename", path.c_str());
  UIContext::instance()->executeCommand(command, params);
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
  auto it = recent->paths_begin();
  auto end = recent->paths_end();
  for (; it != end; ++it)
    addChild(new RecentFileItem(*it));
}

void RecentFoldersListBox::onClick(const std::string& path)
{
  Command* command = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
  Params params;
  params.set("folder", path.c_str());
  UIContext::instance()->executeCommand(command, params);
}

} // namespace app
