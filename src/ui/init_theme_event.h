// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_INIT_THEME_EVENT_H_INCLUDED
#define UI_INIT_THEME_EVENT_H_INCLUDED

#include "ui/event.h"

namespace ui {

  class Theme;

  class InitThemeEvent : public Event
  {
  public:
    InitThemeEvent(Component* source, Theme* theme)
      : Event(source)
      , m_theme(theme){ }

    Theme* getTheme() const { return m_theme; }

  private:
    Theme* m_theme;
  };

} // namespace ui

#endif  // UI_INIT_THEME_EVENT_H_INCLUDED
