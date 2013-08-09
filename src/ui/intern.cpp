// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/manager.h"
#include "ui/theme.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <list>

namespace ui {

static std::list<Widget*>* widgets;

int _ji_widgets_init()
{
  widgets = new std::list<Widget*>;
  return 0;
}

void _ji_widgets_exit()
{
  delete widgets;
}

void addWidget(Widget* widget)
{
  widgets->push_back(widget);
}

void removeWidget(Widget* widget)
{
  std::list<Widget*>::iterator it =
    std::find(widgets->begin(), widgets->end(), widget);

  if (it != widgets->end())
    widgets->erase(it);
}

void setFontOfAllWidgets(FONT* f)
{
  for (std::list<Widget*>::iterator it=widgets->begin(), end=widgets->end();
       it != end; ++it) {
    (*it)->setFont(f);
  }
}

void reinitThemeForAllWidgets()
{
  // Reinitialize the theme of each widget
  for (std::list<Widget*>::iterator it=widgets->begin(), end=widgets->end();
       it != end; ++it) {
      (*it)->setTheme(CurrentTheme::get());
      (*it)->initTheme();
    }

  // Remap the windows
  for (std::list<Widget*>::iterator it=widgets->begin(), end=widgets->end();
       it != end; ++it) {
    if ((*it)->type == kWindowWidget)
      static_cast<Window*>(*it)->remapWindow();
  }

  // Redraw the whole screen
  Manager::getDefault()->invalidate();
}

} // namespace ui
