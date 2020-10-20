// Aseprite UI Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/manager.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <list>

namespace ui {
namespace details {

static std::list<Widget*>* widgets;

void initWidgets()
{
  assert_ui_thread();

  widgets = new std::list<Widget*>;
}

void exitWidgets()
{
  assert_ui_thread();

  delete widgets;
}

void addWidget(Widget* widget)
{
  assert_ui_thread();

  widgets->push_back(widget);
}

void removeWidget(Widget* widget)
{
  assert_ui_thread();

  ASSERT(!Manager::widgetAssociatedToManager(widget));

  auto it = std::find(widgets->begin(), widgets->end(), widget);
  if (it != widgets->end())
    widgets->erase(it);
}

void reinitThemeForAllWidgets()
{
  assert_ui_thread();

  // Reinitialize the theme of each widget
  auto theme = get_theme();
  for (auto widget : *widgets)
    widget->setTheme(theme);
}

} // namespace details
} // namespace ui
