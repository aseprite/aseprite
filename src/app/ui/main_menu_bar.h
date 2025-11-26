// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_MAIN_MENU_BAR_H_INCLUDED
#define APP_UI_MAIN_MENU_BAR_H_INCLUDED
#pragma once

#include "app/ui/dockable.h"
#include "obs/connection.h"
#include "ui/menu.h"

namespace app {

class MainMenuBar : public ui::MenuBar,
                    public Dockable {
public:
  MainMenuBar();

  void queueReload();
  void reload();

  // Dockable impl
  int dockableAt() const override { return ui::TOP | ui::BOTTOM; }

private:
  obs::scoped_connection m_extKeys;
  obs::scoped_connection m_extScripts;
  bool m_queuedReload = false;
};

} // namespace app

#endif
