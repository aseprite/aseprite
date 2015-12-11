// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/user_data_popup.h"

#include "doc/user_data.h"

#include "user_data.xml.h"

namespace app {

using namespace ui;

namespace {

class UserDataPopup : public app::gen::UserData {
public:
  UserDataPopup() {
    makeFixed();
    setClickBehavior(ClickBehavior::CloseOnClickInOtherWindow);
    setEnterBehavior(EnterBehavior::CloseOnEnter);
    setCloseOnKeyDown(false);

    // The whole manager as hot region
    setHotRegion(gfx::Region(manager()->bounds()));
  }
};

} // anonymous namespace

bool show_user_data_popup(const gfx::Rect& bounds,
                          doc::UserData& userData)
{
  UserDataPopup window;

  window.text()->setText(userData.text());
  window.pointAt(TOP, bounds);

  window.openWindowInForeground();

  if (userData.text() != window.text()->text()) {
    userData.setText(window.text()->text());
    return true;
  }
  else {
    return  false;
  }
}

} // namespace app
