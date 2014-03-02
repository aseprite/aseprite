// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef BASE_FS_H_INCLUDED
#define BASE_FS_H_INCLUDED

#include "base/string.h"

namespace base {

  bool file_exists(const string& path);
  bool directory_exists(const string& path);

  void delete_file(const string& path);

  bool has_readonly_attr(const string& path);
  void remove_readonly_attr(const string& path);

  void make_directory(const string& path);
  void remove_directory(const string& path);

  string get_app_path();
  string get_temp_path();

}

#endif
