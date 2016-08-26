// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/recent_files.h"

#include "app/ini_file.h"
#include "base/fs.h"
#include "base/path.h"

#include <cstdio>
#include <cstring>
#include <set>

namespace {

struct compare_path {
  std::string a;
  compare_path(const std::string& a) : a(a) { }
  bool operator()(const std::string& b) const {
    return base::compare_filenames(a, b) == 0;
  }
};

}

namespace app {

RecentFiles::RecentFiles()
  : m_files(16)
  , m_paths(16)
{
  char buf[512];

  for (int c=m_files.limit()-1; c>=0; c--) {
    sprintf(buf, "Filename%02d", c);

    const char* filename = get_config_string("RecentFiles", buf, NULL);
    if (filename && *filename && base::is_file(filename)) {
      std::string fn = normalizePath(filename);
      m_files.addItem(fn, compare_path(fn));
    }
  }

  for (int c=m_paths.limit()-1; c>=0; c--) {
    sprintf(buf, "Path%02d", c);

    const char* path = get_config_string("RecentPaths", buf, NULL);
    if (path && *path) {
      std::string p = normalizePath(path);
      m_paths.addItem(p, compare_path(p));
    }
  }
}

RecentFiles::~RecentFiles()
{
  char buf[512];

  int c = 0;
  for (auto const& filename : m_files) {
    sprintf(buf, "Filename%02d", c);
    set_config_string("RecentFiles", buf, filename.c_str());
    c++;
  }

  c = 0;
  for (auto const& path : m_paths) {
    sprintf(buf, "Path%02d", c);
    set_config_string("RecentPaths", buf, path.c_str());
    c++;
  }
}

void RecentFiles::addRecentFile(const char* filename)
{
  std::string fn = normalizePath(filename);
  m_files.addItem(fn, compare_path(fn));

  std::string path = base::get_file_path(fn);
  m_paths.addItem(path, compare_path(path));

  Changed();
}

void RecentFiles::removeRecentFile(const char* filename)
{
  std::string fn = normalizePath(filename);
  m_files.removeItem(fn, compare_path(fn));

  std::string path = base::get_file_path(filename);
  m_paths.removeItem(path, compare_path(path));

  Changed();
}

std::string RecentFiles::normalizePath(std::string fn)
{
  return base::normalize_path(fn);
}

} // namespace app
