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

  // Key=theme/palette/etc. id
  // Value=theme/palette/etc. path
  typedef std::map<std::string, std::string> ExtensionItems;

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

    const ExtensionItems& themes() const { return m_themes; }
    const ExtensionItems& palettes() const { return m_palettes; }

    void addTheme(const std::string& id, const std::string& path);
    void addPalette(const std::string& id, const std::string& path);

    bool isEnabled() const { return m_isEnabled; }
    bool isInstalled() const { return m_isInstalled; }
    bool canBeDisabled() const;
    bool canBeUninstalled() const;

    void enable(const bool state);
    void uninstall();

    obs::signal<void(Extension*, bool)> Enable;
    obs::signal<void(Extension*)> Disable;
    obs::signal<void(Extension*)> Uninstall;

  private:
    void uninstallFiles(const std::string& path);
    bool isCurrentTheme() const;

    ExtensionItems m_themes;
    ExtensionItems m_palettes;
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

    Extension* installCompressedExtension(const std::string& zipFn);

    std::string themePath(const std::string& themeId);
    ExtensionItems palettes() const;

    obs::signal<void(Extension*)> NewExtension;

  private:
    Extension* loadExtension(const std::string& path,
                             const std::string& fullPackageFilename,
                             const bool isBuiltinExtension);

    List m_extensions;
    std::string m_userExtensionsPath;
  };

} // namespace app

#endif
