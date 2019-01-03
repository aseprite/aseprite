// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_APP_MOD_H_INCLUDED
#define APP_APP_MOD_H_INCLUDED
#pragma once

namespace app {
  class MainWindow;

  class AppMod {
  public:
    virtual ~AppMod() { }
    virtual void modMainWindow(MainWindow* mainWindow) = 0;
  };

} // namespace app

#endif
