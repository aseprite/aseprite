// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_WIN32_EXCEPTION_H_INCLUDED
#define BASE_WIN32_EXCEPTION_H_INCLUDED
#pragma once

#include "exception.h"

namespace base {

  class Win32Exception : public Exception {
  public:
    Win32Exception(const std::string& msg) throw();
    virtual ~Win32Exception() throw();
  };

}

#endif
