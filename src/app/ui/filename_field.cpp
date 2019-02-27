// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/filename_field.h"

#include "app/i18n/strings.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "base/fs.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/entry.h"
#include "ui/message.h"

namespace app {

using namespace ui;

FilenameField::FilenameField(const Type type,
                             const std::string& pathAndFilename)
  : m_entry(type == EntryAndButton ? new ui::Entry(1024, ""): nullptr)
  , m_button(type == EntryAndButton ? Strings::select_file_browse():
                                      Strings::select_file_text())
{
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

  if (m_entry) {
    m_entry->Change.connect(
      base::Bind<void>(
        [this]{
          m_file = m_entry->text();
          Change();
        }));
  }

  m_button.Click.connect(
    base::Bind<void>(
      [this]{
        std::string fn = SelectFile();
        if (!fn.empty()) {
          setFilename(fn);
        }
      }));

  initTheme();
}

std::string FilenameField::filename() const
{
  return base::join_path(m_path, m_file);
}

void FilenameField::setFilename(const std::string& fn)
{
  if (filename() == fn)
    return;

  m_path = base::get_file_path(fn);
  m_file = base::get_file_name(fn);
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

  auto theme = skin::SkinTheme::instance();
  ui::Style* style = theme->styles.miniButton();
  if (style)
    m_button.setStyle(style);
}

void FilenameField::updateWidgets()
{
  if (m_entry)
    m_entry->setText(m_file);
  else if (m_file.empty())
    m_button.setText(Strings::select_file_text());
  else
    m_button.setText(m_file);

  Change();
}

} // namespace app
