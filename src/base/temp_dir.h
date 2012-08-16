// ASEPRITE base library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_TEMP_DIR_H_INCLUDED
#define BASE_TEMP_DIR_H_INCLUDED

#include "base/string.h"

namespace base {

  class TempDir {
  public:
    TempDir();
    TempDir(const string& appName);
    ~TempDir();

    void attach(const string& path);
    const string& path() const { return m_path; }

  private:
    string m_path;
  };

}

#endif
