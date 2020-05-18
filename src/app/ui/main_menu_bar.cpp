// Aseprite
// Copyright (C) 2020  Igara Studio S.A
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/main_menu_bar.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/extensions.h"
#include "base/bind.h"

namespace app {

MainMenuBar::MainMenuBar()
{
  Extensions& extensions = App::instance()->extensions();

  m_extScripts =
    extensions.ScriptsChange.connect(
      base::Bind<void>(&MainMenuBar::reload, this));
}

void MainMenuBar::reload()
{
  setMenu(nullptr);

  // Reload all menus.
  AppMenus::instance()->reload();

  setMenu(AppMenus::instance()->getRootMenu());
  if (auto p = parent())
    p->layout();
}

} // namespace app
