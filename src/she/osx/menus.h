// SHE library
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_OSX_MENUS_H_INCLUDED
#define SHE_OSX_MENUS_H_INCLUDED
#pragma once

#include "she/menus.h"

namespace she {

  class MenusOSX : public Menus {
  public:
    MenusOSX();
    Menu* createMenu() override;
    MenuItem* createMenuItem(const MenuItemInfo& info) override;
    void setAppMenu(Menu* menu) override;
  };

} // namespace she

#endif
