// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_FILTERS_CELS_TARGET_H_INCLUDED
#define APP_COMMANDS_FILTERS_CELS_TARGET_H_INCLUDED
#pragma once

namespace app {

  enum class CelsTarget {
    Selected,                 // Selected cels in the timeline
    All                       // All cels in the sprite
  };

} // namespace app

#endif
