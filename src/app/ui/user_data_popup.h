// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
