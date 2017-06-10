// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_EXTENSIONS_H_INCLUDED
#define APP_EXTENSIONS_H_INCLUDED
#pragma once

#include "obs/signal.h"

#include <map>
#include <string>
#include <vector>

namespace app {

  class Extension {
  public:
    Extension(const std::string& path,
              const std::string& name,
              const std::string& displayName,
              const bool isEnabled,
              const bool isBuiltinExtension);

    const std::string& path() const { return m_path; }
    const std::string& name() const { return m_name; }
    const std::string& displayName() const { return m_displayName; }

    bool isEnabled() const { return m_isEnabled; }
    bool isInstalled() const { return m_isInstalled; }
    bool canBeDisabled() const { return m_isEnabled; }
    bool canBeUninstalled() const { return !m_isBuiltinExtension; }

    void enable(const bool state);
    void uninstall();

    obs::signal<void(Extension*, bool)> Enable;
    obs::signal<void(Extension*)> Disable;
    obs::signal<void(Extension*)> Uninstall;

  private:
    std::string m_path;
    std::string m_name;
    std::string m_displayName;
    bool m_isEnabled;
    bool m_isInstalled;
    bool m_isBuiltinExtension;
  };

  class Extensions {
  public:
    typedef std::vector<Extension*> List;
    typedef List::iterator iterator;

    Extensions();
    ~Extensions();

    iterator begin() { return m_extensions.begin(); }
    iterator end() { return m_extensions.end(); }

    void enableExtension(Extension* extension);
    void disableExtension(Extension* extension);
    void uninstallExtension(Extension* extension);

    void installCompressedExtension(const std::string& zipFn);

    std::string themePath(const std::string& themeId);

  private:
    Extension* loadExtension(const std::string& path,
                             const std::string& fullPackageFilename,
                             const bool isBuiltinExtension);

    List m_extensions;

    // Key=theme id, Value=theme path
    std::map<std::string, std::string> m_builtinThemes;
    std::map<std::string, std::string> m_userThemes;
  };

} // namespace app

#endif
