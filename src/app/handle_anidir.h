// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_HANDLE_ANIDIR_H_INCLUDED
#define APP_HANDLE_ANIDIR_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "doc/frame.h"

namespace doc {
  class Sprite;
}

namespace app {

  doc::frame_t calculate_next_frame(
    doc::Sprite* sprite,
    doc::frame_t frame,
    DocumentPreferences& docPref,
    bool& pingPongForward);

} // namespace app

#endif
