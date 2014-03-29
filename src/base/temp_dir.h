// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_TEMP_DIR_H_INCLUDED
#define BASE_TEMP_DIR_H_INCLUDED
#pragma once

#include "base/string.h"

namespace base {

  class TempDir {
  public:
    TempDir();
    TempDir(const string& appName);
    ~TempDir();

    void remove();
    void attach(const string& path);
    const string& path() const { return m_path; }

  private:
    string m_path;
  };

}

#endif
