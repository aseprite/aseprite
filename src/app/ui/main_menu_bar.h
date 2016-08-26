// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
