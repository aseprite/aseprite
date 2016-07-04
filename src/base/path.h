// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_PATH_H_INCLUDED
#define BASE_PATH_H_INCLUDED
#pragma once

#include <string>

namespace base {

  // Default path separator (on Windows it is '\' and on Unix-like systems it is '/').
  extern const std::string::value_type path_separator;

  // Returns true if the given character is a valud path separator
  // (any of '\' or '/' characters).
  bool is_path_separator(std::string::value_type chr);

  // Returns only the path (without the last trailing slash).
  std::string get_file_path(const std::string& filename);

  // Returns the file name with its extension, removing the path.
  std::string get_file_name(const std::string& filename);

  // Returns the extension of the file name (without the dot).
  std::string get_file_extension(const std::string& filename);

  // Returns the whole path with another extension.
  std::string replace_extension(const std::string& filename, const std::string& extension);

  // Returns the file name without path and without extension.
  std::string get_file_title(const std::string& filename);

  // Joins two paths or a path and a file name with a path-separator.
  std::string join_path(const std::string& path, const std::string& file);

  // Removes the trailing separator from the given path.
  std::string remove_path_separator(const std::string& path);

  // Replaces all separators with the system separator.
  std::string fix_path_separators(const std::string& filename);

  // Calls get_canonical_path() and fix_path_separators() for the
  // given filename.
  std::string normalize_path(const std::string& filename);

  // Returns true if the filename contains one of the specified
  // extensions. The cvs_extensions parameter must be a set of
  // possible extensions separated by comma.
  bool has_file_extension(const std::string& filename, const std::string& csv_extensions);

  int compare_filenames(const std::string& a, const std::string& b);

}

#endif
