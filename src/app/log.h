// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_LOG_H_INCLUDED
#define APP_LOG_H_INCLUDED
#pragma once

namespace app {

class LoggerModule {
public:
  LoggerModule(bool createLogInDesktop);
  ~LoggerModule();
};

} // namespace app

#endif
