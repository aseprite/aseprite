// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
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

enum { kPinnedFiles, kRecentFiles,
       kPinnedPaths, kRecentPaths };

const char* kSectionName[] = { "PinnedFiles",
                               "RecentFiles",
                               "PinnedPaths",
                               "RecentPaths" };

// Special key used in recent sections (files/paths) to indicate that
// the section was already converted at least one time.
const char* kConversionKey = "_";

struct compare_path {
  std::string a;
  compare_path(const std::string& a) : a(a) { }
  bool operator()(const std::string& b) const {
    return base::compare_filenames(a, b) == 0;
  }
};

}

namespace app {

RecentFiles::RecentFiles(const int limit)
  : m_limit(limit)
{
  load();
}

RecentFiles::~RecentFiles()
{
  save();
}

void RecentFiles::addRecentFile(const std::string& filename)
{
  std::string fn = normalizePath(filename);

  // If the filename is already pinned, we don't add it in the
  // collection of recent files collection.
  auto it = std::find(m_paths[kPinnedFiles].begin(),
                      m_paths[kPinnedFiles].end(), fn);
  if (it != m_paths[kPinnedFiles].end())
    return;
  addItem(m_paths[kRecentFiles], fn);

  // Add recent folder
  std::string path = base::get_file_path(fn);
  it = std::find(m_paths[kPinnedFolders].begin(),
                 m_paths[kPinnedFolders].end(), path);
  if (it == m_paths[kPinnedFolders].end()) {
    addItem(m_paths[kRecentFolders], path);
  }

  Changed();
}

void RecentFiles::removeRecentFile(const std::string& filename)
{
  std::string fn = normalizePath(filename);
  removeItem(m_paths[kRecentFiles], fn);

  std::string dir = base::get_file_path(fn);
  if (!base::is_directory(dir))
    removeRecentFolder(dir);

  Changed();
}

void RecentFiles::removeRecentFolder(const std::string& dir)
{
  std::string fn = normalizePath(dir);
  removeItem(m_paths[kRecentFolders], fn);

  Changed();
}

void RecentFiles::setLimit(const int newLimit)
{
  ASSERT(newLimit >= 0);

  for (auto& list : m_paths) {
    if (newLimit < list.size()) {
      auto it = list.begin();
      std::advance(it, newLimit);
      list.erase(it, list.end());
    }
  }

  m_limit = newLimit;
  Changed();
}

void RecentFiles::clear()
{
  // Clear only recent items (not pinned items)
  m_paths[kRecentFiles].clear();
  m_paths[kRecentFolders].clear();

  Changed();
}

void RecentFiles::setFiles(const base::paths& pinnedFiles,
                           const base::paths& recentFiles)
{
  m_paths[kPinnedFiles] = pinnedFiles;
  m_paths[kRecentFiles] = recentFiles;
}

void RecentFiles::setFolders(const base::paths& pinnedFolders,
                             const base::paths& recentFolders)
{
  m_paths[kPinnedFolders] = pinnedFolders;
  m_paths[kRecentFolders] = recentFolders;
}

std::string RecentFiles::normalizePath(const std::string& filename)
{
  return base::normalize_path(filename);
}

void RecentFiles::addItem(base::paths& list, const std::string& fn)
{
  auto it = std::find_if(list.begin(), list.end(), compare_path(fn));

  // If the item already exist in the list...
  if (it != list.end()) {
    // Move it to the first position
    list.erase(it);
    list.insert(list.begin(), fn);
    return;
  }

  if (m_limit > 0)
    list.insert(list.begin(), fn);

  while (list.size() > m_limit)
    list.erase(--list.end());
}

void RecentFiles::removeItem(base::paths& list, const std::string& fn)
{
  auto it = std::find_if(list.begin(), list.end(), compare_path(fn));
  if (it != list.end())
    list.erase(it);
}

void RecentFiles::load()
{
  for (int i=0; i<kCollections; ++i) {
    const char* section = kSectionName[i];

    // For recent files: If there is an item called "Filename00" and no "0" key
    // For recent paths: If there is an item called "Path00" and no "0" key
    // -> We are migrating from and old version to a new one
    const bool processOldFilenames =
      (i == kRecentFiles &&
       get_config_string(section, "Filename00", nullptr) &&
       !get_config_bool(section, kConversionKey, false));

    const bool processOldPaths =
      (i == kRecentPaths &&
       get_config_string(section, "Path00", nullptr) &&
       !get_config_bool(section, kConversionKey, false));

    for (const auto& key : enum_config_keys(section)) {
      if ((!processOldFilenames && std::strncmp(key.c_str(), "Filename", 8) == 0)
          ||
          (!processOldPaths && std::strncmp(key.c_str(), "Path", 4) == 0)) {
        // Ignore old entries if we are going to read the new ones
        continue;
      }

      const char* fn = get_config_string(section, key.c_str(), nullptr);
      if (fn && *fn &&
          ((i < 2 && base::is_file(fn)) ||
           (i >= 2 && base::is_directory(fn)))) {
        std::string normalFn = normalizePath(fn);
        m_paths[i].push_back(normalFn);
      }
    }
  }
}

void RecentFiles::save()
{
  for (int i=0; i<kCollections; ++i) {
    const char* section = kSectionName[i];

    for (const auto& key : enum_config_keys(section)) {
      if ((i == kRecentFiles &&
           (std::strncmp(key.c_str(), "Filename", 8) == 0 || key == kConversionKey))
          ||
          (i == kRecentPaths &&
           (std::strncmp(key.c_str(), "Path", 4) == 0 || key == kConversionKey))) {
        // Ignore old entries if we are going to read the new ones
        continue;
      }
      del_config_value(section, key.c_str());
    }

    for (int j=0; j<m_paths[i].size(); ++j) {
      set_config_string(section,
                        fmt::format("{:04d}", j).c_str(),
                        m_paths[i][j].c_str());
    }
    // Special entry that indicates that we've already converted
    if ((i == kRecentFiles || i == kRecentPaths) &&
        !get_config_bool(section, kConversionKey, false)) {
      set_config_bool(section, kConversionKey, true);
    }
  }
}

} // namespace app
