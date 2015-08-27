// Aseprite Base Library
// Copyright (c) 2001-2013, 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_FS_H_INCLUDED
#define BASE_FS_H_INCLUDED
#pragma once

#include <string>
#include <vector>

namespace base {

  class Time;

  bool is_file(const std::string& path);
  bool is_directory(const std::string& path);

  size_t file_size(const std::string& path);

  void move_file(const std::string& src, const std::string& dst);
  void delete_file(const std::string& path);

  bool has_readonly_attr(const std::string& path);
  void remove_readonly_attr(const std::string& path);

  Time get_modification_time(const std::string& path);

  void make_directory(const std::string& path);
  void make_all_directories(const std::string& path);
  void remove_directory(const std::string& path);

  std::string get_current_path();
  std::string get_app_path();
  std::string get_temp_path();
  std::string get_user_docs_folder();

  // If the given filename is a relative path, it converts the
  // filename to an absolute one.
  std::string get_canonical_path(const std::string& path);

  std::vector<std::string> list_files(const std::string& path);

} // namespace base

#endif
