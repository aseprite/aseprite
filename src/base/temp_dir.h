// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_TEMP_DIR_H_INCLUDED
#define BASE_TEMP_DIR_H_INCLUDED
#pragma once

#include <string>

namespace base {

  class TempDir {
  public:
    TempDir();
    TempDir(const std::string& appName);
    ~TempDir();

    void remove();
    void attach(const std::string& path);
    const std::string& path() const { return m_path; }

  private:
    std::string m_path;
  };

}

#endif
