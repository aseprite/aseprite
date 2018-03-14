// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/recent_files.h"

#include "app/ini_file.h"
#include "base/fs.h"
#include "fmt/format.h"

#include <cstdio>
#include <cstring>
#include <set>

namespace {

const char* kRecentFilesSection = "RecentFiles";
const char* kRecentFoldersSection = "RecentPaths";

struct compare_path {
  std::string a;
  compare_path(const std::string& a) : a(a) { }
  bool operator()(const std::string& b) const {
    return base::compare_filenames(a, b) == 0;
  }
};

std::string format_filename_var(const int i) {
  return fmt::format("Filename{0:02d}", i);
}

std::string format_folder_var(const int i) {
  return fmt::format("Path{0:02d}", i);
}

}

namespace app {

RecentFiles::RecentFiles()
  : m_files(16)
  , m_paths(16)
{
  for (int c=m_files.limit()-1; c>=0; c--) {
    const char* filename = get_config_string(kRecentFilesSection,
                                             format_filename_var(c).c_str(),
                                             nullptr);
    if (filename && *filename && base::is_file(filename)) {
      std::string fn = normalizePath(filename);
      m_files.addItem(fn, compare_path(fn));
    }
  }

  for (int c=m_paths.limit()-1; c>=0; c--) {
    const char* path = get_config_string(kRecentFoldersSection,
                                         format_folder_var(c).c_str(),
                                         nullptr);
    if (path && *path) {
      std::string p = normalizePath(path);
      m_paths.addItem(p, compare_path(p));
    }
  }
}

RecentFiles::~RecentFiles()
{
  // Save recent files

  int c = 0;
  for (auto const& filename : m_files) {
    set_config_string(kRecentFilesSection,
                      format_filename_var(c).c_str(),
                      filename.c_str());
    ++c;
  }
  for (; c<m_files.limit(); ++c) {
    del_config_value(kRecentFilesSection,
                     format_filename_var(c).c_str());
  }

  // Save recent folders

  c = 0;
  for (auto const& path : m_paths) {
    set_config_string(kRecentFoldersSection,
                      format_folder_var(c).c_str(),
                      path.c_str());
    ++c;
  }
  for (; c<m_files.limit(); ++c) {
    del_config_value(kRecentFoldersSection,
                     format_folder_var(c).c_str());
  }
}

void RecentFiles::addRecentFile(const std::string& filename)
{
  std::string fn = normalizePath(filename);
  m_files.addItem(fn, compare_path(fn));

  std::string path = base::get_file_path(fn);
  m_paths.addItem(path, compare_path(path));

  Changed();
}

void RecentFiles::removeRecentFile(const std::string& filename)
{
  std::string fn = normalizePath(filename);
  m_files.removeItem(fn, compare_path(fn));

  std::string dir = base::get_file_path(fn);
  if (!base::is_directory(dir))
    removeRecentFolder(dir);

  Changed();
}

void RecentFiles::removeRecentFolder(const std::string& dir)
{
  std::string fn = normalizePath(dir);
  m_paths.removeItem(fn, compare_path(fn));

  Changed();
}

std::string RecentFiles::normalizePath(const std::string& filename)
{
  return base::normalize_path(filename);
}

} // namespace app
