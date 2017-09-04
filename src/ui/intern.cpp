// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/manager.h"
#include "ui/theme.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <list>

namespace ui {
namespace details {

static std::list<Widget*>* widgets;

void initWidgets()
{
  widgets = new std::list<Widget*>;
}

void exitWidgets()
{
  delete widgets;
}

void addWidget(Widget* widget)
{
  widgets->push_back(widget);
}

void removeWidget(Widget* widget)
{
  ASSERT(!Manager::widgetAssociatedToManager(widget));

  auto it = std::find(widgets->begin(), widgets->end(), widget);
  if (it != widgets->end())
    widgets->erase(it);
}

void reinitThemeForAllWidgets()
{
  // Reinitialize the theme of each widget
  auto theme = get_theme();
  for (auto widget : *widgets)
    widget->setTheme(theme);
}

} // namespace details
} // namespace ui
