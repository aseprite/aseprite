// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_FILE_FORMAT_OPTIONS_H_INCLUDED
#define APP_FILE_FORMAT_OPTIONS_H_INCLUDED
#pragma once

#include <memory>

namespace app {

  // Extra options loaded from a file that can be useful to save the
  // file later in the same loaded format (e.g. same color depth, same
  // jpeg quality, etc.).
  class FormatOptions {
  public:
    virtual ~FormatOptions() { }
  };

  typedef std::shared_ptr<FormatOptions> FormatOptionsPtr;

} // namespace app

#endif
