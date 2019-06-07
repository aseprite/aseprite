// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CRASH_RECOVERY_CONFIG_H_INCLUDED
#define APP_CRASH_RECOVERY_CONFIG_H_INCLUDED
#pragma once

namespace app {
namespace crash {

  // Structure to store the configuration from Preferences instance to
  // avoid accessing to Preferences from a non-UI thread.
  struct RecoveryConfig {
    double dataRecoveryPeriod;
    int keepEditedSpriteDataFor;
  };

} // namespace crash
} // namespace app

#endif
