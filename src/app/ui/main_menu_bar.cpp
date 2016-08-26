// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/main_menu_bar.h"

#include "app/app_menus.h"

namespace app {

MainMenuBar::MainMenuBar()
{
}

void MainMenuBar::reload()
{
  setMenu(NULL);

  // Reload all menus.
  AppMenus::instance()->reload();

  setMenu(AppMenus::instance()->getRootMenu());
}

} // namespace app
