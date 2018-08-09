// LAF OS Library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_LOGGER_H_INCLUDED
#define OS_LOGGER_H_INCLUDED
#pragma once

namespace os {

  class Logger {
  public:
    virtual ~Logger() { }
    virtual void logError(const char* error) = 0;
  };

} // namespace os

#endif
