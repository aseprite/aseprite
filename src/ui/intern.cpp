// Aseprite UI Library
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/utf8_decode.h"
#include "ui/manager.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <memory>
#include <set>

namespace ui {
namespace details {

static std::unique_ptr<std::set<Widget*>> widgets;

void initWidgets()
{
  assert_ui_thread();
  widgets = std::make_unique<std::set<Widget*>>();
}

void exitWidgets()
{
  assert_ui_thread();
  widgets.reset();
}

void addWidget(Widget* widget)
{
  assert_ui_thread();

  widgets->insert(widget);
}

void removeWidget(Widget* widget)
{
  assert_ui_thread();

  ASSERT(!Manager::widgetAssociatedToManager(widget));

  widgets->erase(widget);
}

// TODO we should be able to re-initialize all widgets without using
//      this global "widgets" set, so we don't have to keep track of
//      all widgets globally
void reinitThemeForAllWidgets()
{
  assert_ui_thread();

  // Reinitialize the theme in this order:
  // 1. From the manager to children (windows and children widgets in
  //    the window etc.)
  auto theme = get_theme();
  if (auto man = Manager::getDefault()) {
    man->setTheme(theme);
  }

  // 2. If some other widget wasn't updated (e.g. is outside the
  // hierarchy of widgets), we update the theme for that one too.
  //
  // TODO Is this really needed?
  for (auto widget : *widgets) {
    if (theme != widget->theme())
      widget->setTheme(theme);
  }
}

std::string removeEscapeCharFromText(const std::string& original,
                                     const int escapeChar)
{
  std::wstring newText; // wstring is used to properly push_back() multibyte chars
  newText.reserve(original.size());

  base::utf8_decode decode(original);
  while (int chr = decode.next()) {
    if (chr == escapeChar) {
      chr = decode.next();
      if (!chr)
        break;    // Ill-formed string (it ends with escape character)
    }
    newText.push_back(chr);
  }
  return base::to_utf8(newText);
}

} // namespace details
} // namespace ui
