// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_INIT_THEME_EVENT_H_INCLUDED
#define GUI_INIT_THEME_EVENT_H_INCLUDED

#include "gui/event.h"

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

#endif  // GUI_INIT_THEME_EVENT_H_INCLUDED
