// SHE library
// Copyright (C) 2012-2014  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_LOGGER_H_INCLUDED
#define SHE_LOGGER_H_INCLUDED
#pragma once

namespace she {

  class Logger {
  public:
    virtual ~Logger() { }
    virtual void logError(const char* error) = 0;
  };

} // namespace she

#endif
