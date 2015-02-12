// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_MAIN_MENU_BAR_H_INCLUDED
#define APP_UI_MAIN_MENU_BAR_H_INCLUDED
#pragma once

#include "ui/menu.h"

namespace app {

  class MainMenuBar : public ui::MenuBar {
  public:
    MainMenuBar();

    void reload();
  };

} // namespace app

#endif
