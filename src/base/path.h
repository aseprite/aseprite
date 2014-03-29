// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef BASE_PATH_H_INCLUDED
#define BASE_PATH_H_INCLUDED
#pragma once

#include "base/string.h"

namespace base {

  // Default path separator (on Windows it is '\' and on Unix-like systems it is '/').
  extern const string::value_type path_separator;

  // Returns true if the given character is a valud path separator
  // (any of '\' or '/' characters).
  bool is_path_separator(string::value_type chr);

  // Returns only the path (without the last trailing slash).
  string get_file_path(const string& filename);

  // Returns the file name with its extension, removing the path.
  string get_file_name(const string& filename);

  // Returns the extension of the file name (without the dot).
  string get_file_extension(const string& filename);

  // Returns the file name without path and without extension.
  string get_file_title(const string& filename);

  // Joins two paths or a path and a file name with a path-separator.
  string join_path(const string& path, const string& file);

  // Removes the trailing separator from the given path.
  string remove_path_separator(const string& path);

  // Replaces all separators with the system separator.
  string fix_path_separators(const string& filename);

  // Returns true if the filename contains one of the specified
  // extensions. The cvs_extensions parameter must be a set of
  // possible extensions separated by comma.
  bool has_file_extension(const string& filename, const string& csv_extensions);

}

#endif
