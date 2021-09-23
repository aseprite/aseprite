// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SENTRY_WRAPPER_H
#define APP_SENTRY_WRAPPER_H

#if !ENABLE_SENTRY
  #error ENABLE_SENTRY must be defined
#endif

#include "sentry.h"

namespace app {

class Sentry {
public:
  void init();
  ~Sentry();

private:
  void setupDirs(sentry_options_t* options);

  bool m_init = false;
};

} // namespace app

#endif  // APP_SENTRY_WRAPPER_H
