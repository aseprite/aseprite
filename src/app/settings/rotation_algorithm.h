// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_SETTINGS_ROTATION_ALGORITHM_H_INCLUDED
#define APP_SETTINGS_ROTATION_ALGORITHM_H_INCLUDED
#pragma once

namespace app {

  enum RotationAlgorithm {
    kFastRotationAlgorithm,
    kRotSpriteRotationAlgorithm,

    kFirstRotationAlgorithm = kFastRotationAlgorithm,
    kLastRotationAlgorithm = kRotSpriteRotationAlgorithm
  };

} // namespace app

#endif
