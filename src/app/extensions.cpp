// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/extensions.h"

#include "app/resource_finder.h"
#include "base/fs.h"
#include "base/unique_ptr.h"

#include "tao/json.hpp"

namespace app {

Extension::Extension(const std::string& path,
                     const std::string& name,
                     const std::string& displayName,
                     const bool isEnabled,
                     const bool isBuiltinExtension)
  : m_path(path)
  , m_name(name)
  , m_displayName(displayName)
  , m_isEnabled(isEnabled)
  , m_isInstalled(true)
  , m_isBuiltinExtension(isBuiltinExtension)
{
}

void Extension::enable(const bool state)
{
  // Do nothing
  if (m_isEnabled == state)
    return;

  // TODO save the enable/disable state on configuration or other place

  m_isEnabled = state;
  Enable(this, state);
}

void Extension::uninstall()
{
  if (!m_isInstalled)
    return;

  // TODO remove files if the extension is not built-in

  m_isEnabled = false;
  m_isInstalled = false;
  Uninstall(this);
}

Extensions::Extensions()
{
  ResourceFinder rf;
  rf.includeDataDir("extensions");

  // Load extensions from data/ directory on all possible locations
  // (installed folder and user folder)
  while (rf.next()) {
    auto extensionsDir = rf.filename();

    if (base::is_directory(extensionsDir)) {
      for (auto fn : base::list_files(extensionsDir)) {
        auto dir = base::join_path(extensionsDir, fn);
        if (!base::is_directory(dir))
          continue;

        auto fullFn = base::join_path(dir, "package.json");
        LOG("EXT: Loading extension '%s'...\n", fullFn.c_str());
        if (!base::is_file(fullFn)) {
          LOG("EXT: File '%s' not found\n", fullFn.c_str());
          continue;
        }

        bool isBuiltinExtension = true; // TODO check if the extension is in Aseprite installation folder or the user folder

        try {
          Extension* extension = loadExtension(dir, fullFn, isBuiltinExtension);
          m_extensions.push_back(extension);
        }
        catch (const std::exception& ex) {
          LOG("EXT: Error loading JSON file: %s\n",
              ex.what());
        }
      }
    }
  }
}

Extensions::~Extensions()
{
  for (auto ext : m_extensions)
    delete ext;
}

std::string Extensions::themePath(const std::string& themeId)
{
  auto it = m_userThemes.find(themeId);
  if (it != m_userThemes.end())
    return it->second;

  it = m_builtinThemes.find(themeId);
  if (it != m_builtinThemes.end())
    return it->second;

  return std::string();
}

Extension* Extensions::loadExtension(const std::string& path,
                                     const std::string& fullPackageFilename,
                                     const bool isBuiltinExtension)
{
  auto json = tao::json::parse_file(fullPackageFilename);
  auto name = json["name"].get_string();
  auto displayName = json["displayName"].get_string();

  LOG("EXT: Extension '%s' loaded\n", name.c_str());

  base::UniquePtr<Extension> extension(
    new Extension(path,
                  name,
                  displayName,
                  true, // TODO check if the extension is enabled in the configuration
                  isBuiltinExtension));

  auto contributes = json["contributes"];
  if (contributes.is_object()) {
    auto themes = contributes["themes"];
    if (themes.is_array()) {
      for (const auto& theme : themes.get_array()) {
        auto jsonThemeId = theme.at("id");
        auto jsonThemePath = theme.at("path");

        if (!jsonThemeId.is_string()) {
          LOG("EXT: A theme doesn't have 'id' property\n");
        }
        else if (!jsonThemePath.is_string()) {
          LOG("EXT: Theme '%s' doesn't have 'path' property\n",
              jsonThemeId.get_string().c_str());
        }
        else {
          std::string themeId = jsonThemeId.get_string();
          std::string themePath = jsonThemePath.get_string();

          // The path must be always relative to the extension
          themePath = base::join_path(path, themePath);

          LOG("EXT: New theme '%s' in '%s'\n",
              themeId.c_str(),
              themePath.c_str());

          if (isBuiltinExtension) {
            m_builtinThemes[themeId] = themePath;
          }
          else {
            m_userThemes[themeId] = themePath;
          }
        }
      }
    }
  }

  return extension.release();
}

} // namespace app
