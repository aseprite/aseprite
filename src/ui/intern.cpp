// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

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

void _ji_add_widget(Widget* widget)
{
  widgets->push_back(widget);
}

void _ji_remove_widget(Widget* widget)
{
  std::list<Widget*>::iterator it =
    std::find(widgets->begin(), widgets->end(), widget);

  if (it != widgets->end())
    widgets->erase(it);
}

void _ji_set_font_of_all_widgets(FONT* f)
{
  for (std::list<Widget*>::iterator it=widgets->begin(), end=widgets->end();
       it != end; ++it) {
    (*it)->setFont(f);
  }
}

void _ji_reinit_theme_in_all_widgets()
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
    if ((*it)->type == JI_WINDOW)
      static_cast<Window*>(*it)->remapWindow();
  }

  // Redraw the whole screen
  Manager::getDefault()->invalidate();
}

} // namespace ui
