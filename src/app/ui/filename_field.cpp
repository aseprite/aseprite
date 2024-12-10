// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/filename_field.h"

#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/ui/skin/skin_theme.h"
#include "base/fs.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/entry.h"
#include "ui/menu.h"
#include "ui/message.h"

namespace app {

using namespace ui;

FilenameField::FilenameField(const Type type, const std::string& pathAndFilename)
  : m_entry(type == EntryAndButton ? new ui::Entry(1024, "") : nullptr)
  , m_button(type == EntryAndButton ? Strings::select_file_browse() : Strings::select_file_text())
  , m_askOverwrite(true)
{
  m_showFullPath = Preferences::instance().general.showFullPath();
  setFocusStop(true);
  if (m_entry) {
    m_entry->setExpansive(true);
    addChild(m_entry);
  }
  else {
    m_button.setExpansive(true);
  }
  addChild(&m_button);

  setFilename(pathAndFilename);

  if (m_entry)
    m_entry->Change.connect([this] { setFilename(updatedFilename()); });

  m_button.Click.connect([this] { onBrowse(); });
  initTheme();
}

std::string FilenameField::fullFilename() const
{
  return base::join_path(m_path, m_file);
}

std::string FilenameField::displayedFilename() const
{
  return (m_showFullPath ? fullFilename() : filename());
}

const std::string FilenameField::updatedFilename() const
{
  if (!m_entry) {
    if (m_button.text().empty())
      return m_path;
    else
      return fullFilename();
  }
  else {
    return (m_showFullPath ? m_entry->text() : base::join_path(m_path, m_entry->text()));
  }
}

void FilenameField::setShowFullPath(const bool on)
{
  const std::string& fn = updatedFilename();
  m_showFullPath = on;
  setFilename(fn);
}

void FilenameField::onBrowse()
{
  gfx::Rect bounds = m_button.bounds();
  m_button.setSelected(false);

  ui::Menu menu;
  ui::MenuItem choose(Strings::select_file_choose());
  ui::MenuItem relative(Strings::select_file_relative_path(m_pathBase));
  ui::MenuItem absolute(Strings::select_file_absolute_path());
  menu.addChild(&choose);
  menu.addChild(new ui::MenuSeparator());
  menu.addChild(&relative);
  menu.addChild(&absolute);

  choose.Click.connect([this] {
    std::string fn = SelectOutputFile();
    if (!fn.empty()) {
      setFilename(fn);
      m_askOverwrite = false; // Already asked in file selector
    }
  });
  relative.Click.connect([this] { setShowFullPath(false); });
  absolute.Click.connect([this] { setShowFullPath(true); });

  menu.showPopup(gfx::Point(bounds.x, bounds.y2()), display());
}

void FilenameField::setFilename(const std::string& pathAndFilename)
{
  if (m_showFullPath || base::get_file_path(m_docFilename).empty()) {
    m_path = base::get_file_path(pathAndFilename);
    m_file = base::get_file_name(pathAndFilename);
  }
  else {
    m_path = base::get_file_path(m_docFilename);
    m_file = base::get_relative_path(pathAndFilename, base::get_file_path(m_docFilename));

    // Cannot find a relative path (e.g. we selected other drive)
    if (m_file == pathAndFilename) {
      m_path = base::get_file_path(pathAndFilename);
      m_file = base::get_file_name(pathAndFilename);
    }
  }
  if (m_showFullPath)
    m_path = base::get_absolute_path(m_path);
  if (!m_showFullPath || m_pathBase.empty())
    m_pathBase = m_path;

  updateWidgets();
}

bool FilenameField::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case ui::kFocusEnterMessage:
      if (manager()->getFocus() == this) {
        if (m_entry)
          m_entry->requestFocus();
        else
          m_button.requestFocus();
      }
      break;
  }
  return HBox::onProcessMessage(msg);
}

void FilenameField::onInitTheme(ui::InitThemeEvent& ev)
{
  HBox::onInitTheme(ev);
  setChildSpacing(0);

  auto theme = skin::SkinTheme::get(this);
  ui::Style* style = theme->styles.miniButton();
  if (style)
    m_button.setStyle(style);
}

void FilenameField::onUpdateText()
{
  setShowFullPath(m_showFullPath);
}

void FilenameField::updateWidgets()
{
  if (!m_entry) {
    if (m_file.empty())
      m_button.setText(Strings::select_file_text());
    else
      m_button.setText(displayedFilename());
  }
  else {
    m_entry->setText(displayedFilename());
  }

  Change();
}

} // namespace app
