// LAF OS Library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_MENUS_H_INCLUDED
#define OS_MENUS_H_INCLUDED
#pragma once

#include "os/keys.h"
#include "os/shortcut.h"

#include <functional>
#include <string>

namespace os {
  class MenuItem;

  struct MenuItemInfo {
    enum Type {
      Normal,
      Separator
    };

    enum Action {
      UserDefined,

      // macOS standard commands
      Hide,
      HideOthers,
      ShowAll,
      Quit,
      Minimize,
      Zoom,
    };

    Type type;
    Action action;
    std::string text;
    Shortcut shortcut;
    std::function<void()> execute;
    std::function<void(os::MenuItem*)> validate;

    explicit MenuItemInfo(const Type type = Normal,
                          const Action action = UserDefined)
      : type(type)
      , action(action) {
    }

    explicit MenuItemInfo(const char* text,
                          const Action action = UserDefined)
      : type(Normal)
      , action(action)
      , text(text) {
    }
  };

  class Menu;
  class MenuItem;
  class Menus;

  class MenuItem {
  public:
    virtual ~MenuItem() { }
    virtual void dispose() = 0;
    virtual void setText(const std::string& text) = 0;
    virtual void setSubmenu(Menu* submenu) = 0;
    virtual void setEnabled(bool state) = 0;
    virtual void setChecked(bool state) = 0;
    virtual void setShortcut(const Shortcut& shortcut) = 0;
  };

  class Menu {
  public:
    virtual ~Menu() { }
    virtual void dispose() = 0;
    virtual void addItem(MenuItem* item) = 0;
    virtual void insertItem(const int index, MenuItem* item) = 0;
  };

  class Menus {
  public:
    virtual ~Menus() { }
    virtual Menu* createMenu() = 0;
    virtual MenuItem* createMenuItem(const MenuItemInfo& info) = 0;
    virtual void setAppMenu(Menu* menu) = 0;
  };

} // namespace os

#endif
