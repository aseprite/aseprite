// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_EXTENSIONS_H_INCLUDED
#define APP_EXTENSIONS_H_INCLUDED
#pragma once

#include "base/unique_ptr.h"
#include "obs/signal.h"

#include <map>
#include <string>
#include <vector>

namespace render {
  class DitheringMatrix;
}

namespace app {

  // Key=theme/palette/etc. id
  // Value=theme/palette/etc. path
  typedef std::map<std::string, std::string> ExtensionItems;

  class Extensions;

  class Extension {
    friend class Extensions;
  public:
    class DitheringMatrixInfo {
    public:
      DitheringMatrixInfo() : m_matrix(nullptr) { }
      DitheringMatrixInfo(const std::string& path,
                          const std::string& name)
        : m_path(path), m_name(name), m_matrix(nullptr) { }

      const std::string& name() const { return m_name; }
      const render::DitheringMatrix& matrix() const;
      void destroyMatrix();

    private:
      std::string m_path;
      std::string m_name;
      mutable render::DitheringMatrix* m_matrix;
    };

    Extension(const std::string& path,
              const std::string& name,
              const std::string& displayName,
              const bool isEnabled,
              const bool isBuiltinExtension);
    ~Extension();

    const std::string& path() const { return m_path; }
    const std::string& name() const { return m_name; }
    const std::string& displayName() const { return m_displayName; }

    const ExtensionItems& themes() const { return m_themes; }
    const ExtensionItems& palettes() const { return m_palettes; }

    void addTheme(const std::string& id, const std::string& path);
    void addPalette(const std::string& id, const std::string& path);
    void addDitheringMatrix(const std::string& id,
                            const std::string& path,
                            const std::string& name);

    bool isEnabled() const { return m_isEnabled; }
    bool isInstalled() const { return m_isInstalled; }
    bool canBeDisabled() const;
    bool canBeUninstalled() const;

    bool hasThemes() const { return !m_themes.empty(); }
    bool hasPalettes() const { return !m_palettes.empty(); }
    bool hasDitheringMatrices() const { return !m_ditheringMatrices.empty(); }

  private:
    void enable(const bool state);
    void uninstall();
    void uninstallFiles(const std::string& path);
    bool isCurrentTheme() const;
    bool isDefaultTheme() const;

    ExtensionItems m_themes;
    ExtensionItems m_palettes;
    std::map<std::string, DitheringMatrixInfo> m_ditheringMatrices;
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

    void enableExtension(Extension* extension, const bool state);
    void uninstallExtension(Extension* extension);
    Extension* installCompressedExtension(const std::string& zipFn);

    std::string themePath(const std::string& themeId);
    std::string palettePath(const std::string& palId);
    ExtensionItems palettes() const;
    const render::DitheringMatrix* ditheringMatrix(const std::string& matrixId);
    std::vector<Extension::DitheringMatrixInfo> ditheringMatrices();

    obs::signal<void(Extension*)> NewExtension;
    obs::signal<void(Extension*)> ThemesChange;
    obs::signal<void(Extension*)> PalettesChange;
    obs::signal<void(Extension*)> DitheringMatricesChange;

  private:
    Extension* loadExtension(const std::string& path,
                             const std::string& fullPackageFilename,
                             const bool isBuiltinExtension);
    void generateExtensionSignals(Extension* extension);

    List m_extensions;
    std::string m_userExtensionsPath;
  };

} // namespace app

#endif
