// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_USER_DATA_POPUP_H_INCLUDED
#define APP_UI_USER_DATA_POPUP_H_INCLUDED
#pragma once

#include "gfx/rect.h"

namespace doc {
  class UserData;
}

namespace app {

  bool show_user_data_popup(const gfx::Rect& bounds,
                            doc::UserData& userData);

} // namespace app

#endif
