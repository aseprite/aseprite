// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_EXTENSIONS_H_INCLUDED
#define APP_EXTENSIONS_H_INCLUDED
#pragma once

#include "app/i18n/lang_info.h"
#include "obs/signal.h"
#include "render/dithering_matrix.h"

#include <map>
#include <string>
#include <vector>

namespace ui {
class Widget;
}

namespace app {

// Key=id
// Value=path
using ExtensionItems = std::map<std::string, std::string>;

class Extensions;

struct ExtensionInfo {
  std::string name;
  std::string version;
  std::string dstPath;
  std::string commonPath;
  bool defaultTheme = false;
};

enum DeletePluginPref { kNo, kYes };

class Extension {
  friend class Extensions;

public:
  static const char* kAsepriteDefaultThemeExtensionName;
  static const char* kAsepriteDefaultThemeId;

  enum class Category {
    None,
    Keys,
    Languages,
    Themes,
    Scripts,
    Palettes,
    DitheringMatrices,
    Multiple,
    Max
  };

  bool isDefaultTheme() const;

  class DitheringMatrixInfo {
  public:
    DitheringMatrixInfo();
    DitheringMatrixInfo(const std::string& path, const std::string& name);

    const std::string& name() const { return m_name; }
    const render::DitheringMatrix& matrix() const;

  private:
    std::string m_path;
    std::string m_name;
    mutable render::DitheringMatrix m_matrix;
    mutable bool m_loaded = false;
  };

  struct ThemeInfo {
    std::string path;
    std::string variant;

    ThemeInfo() = default;
    ThemeInfo(const std::string& path, const std::string& variant) : path(path), variant(variant) {}
  };

  using Languages = std::map<std::string, LangInfo>;
  using Themes = std::map<std::string, ThemeInfo>;
  using DitheringMatrices = std::map<std::string, DitheringMatrixInfo>;

  Extension(const std::string& path,
            const std::string& name,
            const std::string& version,
            const std::string& displayName,
            const bool isEnabled,
            const bool isBuiltinExtension);
  ~Extension();

  void executeInitActions();
  void executeExitActions();

  const std::string& path() const { return m_path; }
  const std::string& name() const { return m_name; }
  const std::string& version() const { return m_version; }
  const std::string& displayName() const { return m_displayName; }
  const Category category() const { return m_category; }

  const ExtensionItems& keys() const { return m_keys; }
  const Languages& languages() const { return m_languages; }
  const Themes& themes() const { return m_themes; }
  const ExtensionItems& palettes() const { return m_palettes; }

  void addKeys(const std::string& id, const std::string& path);
  void addLanguage(const std::string& id, const std::string& path, const std::string& displayName);
  void addTheme(const std::string& id, const std::string& path, const std::string& variant);
  void addPalette(const std::string& id, const std::string& path);
  void addDitheringMatrix(const std::string& id, const std::string& path, const std::string& name);
#ifdef ENABLE_SCRIPTING
  void addCommand(const std::string& id);
  void removeCommand(const std::string& id);

  void addMenuGroup(const std::string& id);
  void removeMenuGroup(const std::string& id);

  void addMenuSeparator(ui::Widget* widget);
#endif

  bool isEnabled() const { return m_isEnabled; }
  bool isInstalled() const { return m_isInstalled; }
  bool canBeDisabled() const;
  bool canBeUninstalled() const;

  bool hasKeys() const { return !m_keys.empty(); }
  bool hasLanguages() const { return !m_languages.empty(); }
  bool hasThemes() const { return !m_themes.empty(); }
  bool hasPalettes() const { return !m_palettes.empty(); }
  bool hasDitheringMatrices() const { return !m_ditheringMatrices.empty(); }
#ifdef ENABLE_SCRIPTING
  bool hasScripts() const { return !m_plugin.scripts.empty(); }
  void addScript(const std::string& fn);
#endif

  bool isCurrentTheme() const;

private:
  void enable(const bool state);
  void uninstall(const DeletePluginPref delPref);
  void uninstallFiles(const std::string& path, const DeletePluginPref delPref);
  void updateCategory(const Category newCategory);
#ifdef ENABLE_SCRIPTING
  void initScripts();
  void exitScripts();
#endif

  ExtensionItems m_keys;
  Languages m_languages;
  Themes m_themes;
  ExtensionItems m_palettes;
  DitheringMatrices m_ditheringMatrices;

#ifdef ENABLE_SCRIPTING
  struct ScriptItem {
    std::string fn;
    int exitFunctionRef;
    ScriptItem(const std::string& fn);
  };
  struct PluginItem {
    enum Type { Command, MenuGroup, MenuSeparator };
    Type type;
    std::string id;
    ui::Widget* widget = nullptr;
  };
  struct Plugin {
    int pluginRef;
    std::vector<ScriptItem> scripts;
    std::vector<PluginItem> items;
  } m_plugin;
#endif

  std::string m_path;
  std::string m_name;
  std::string m_version;
  std::string m_displayName;
  Category m_category;
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

  void executeInitActions();
  void executeExitActions();

  iterator begin() { return m_extensions.begin(); }
  iterator end() { return m_extensions.end(); }

  void enableExtension(Extension* extension, const bool state);
  void uninstallExtension(Extension* extension, const DeletePluginPref delPref);
  ExtensionInfo getCompressedExtensionInfo(const std::string& zipFn);
  Extension* installCompressedExtension(const std::string& zipFn, const ExtensionInfo& info);

  std::string languagePath(const std::string& langId);
  std::string themePath(const std::string& themeId);
  std::string palettePath(const std::string& palId);
  ExtensionItems palettes() const;
  const render::DitheringMatrix* ditheringMatrix(const std::string& matrixId);

  // The returned collection can be used temporarily while
  // extensions are not installed/uninstalled. Each element is
  // pointing to the real matrix info owned by extensions, this is
  // needed to cache the matrix because it is lazy loaded from an
  // image file. These pointers cannot be deleted.
  std::vector<Extension::DitheringMatrixInfo*> ditheringMatrices();

  obs::signal<void(Extension*)> NewExtension;
  obs::signal<void(Extension*)> KeysChange;
  obs::signal<void(Extension*)> LanguagesChange;
  obs::signal<void(Extension*)> ThemesChange;
  obs::signal<void(Extension*)> PalettesChange;
  obs::signal<void(Extension*)> DitheringMatricesChange;
  obs::signal<void(Extension*)> ScriptsChange;

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
