// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_LOG_H_INCLUDED
#define APP_LOG_H_INCLUDED
#pragma once

namespace app {

  class LoggerModule {
  public:
    LoggerModule(bool verbose);
    ~LoggerModule();

    bool isVerbose() const { return m_verbose; }

  private:
    bool m_verbose;
  };

} // namespace app

#endif
