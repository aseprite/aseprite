// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/extensions.h"

#include "app/ini_file.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "base/exception.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/unique_ptr.h"

#include "archive.h"
#include "archive_entry.h"
#include "tao/json.hpp"

#include <queue>

namespace app {

namespace {

class ReadArchive {
public:
  ReadArchive(const std::string& filename)
    : m_arch(nullptr), m_open(false) {
    m_arch = archive_read_new();
    archive_read_support_format_zip(m_arch);

    m_file = base::open_file(filename, "rb");
    if (!m_file)
      throw base::Exception("Error loading file %s",
                            filename.c_str());

    int err;
    if ((err = archive_read_open_FILE(m_arch, m_file.get())))
      throw base::Exception("Error uncompressing extension\n%s (%d)",
                            archive_error_string(m_arch), err);

    m_open = true;
  }

  ~ReadArchive() {
    if (m_arch) {
      if (m_open)
        archive_read_close(m_arch);
      archive_read_free(m_arch);
    }
  }

  archive_entry* readEntry() {
    archive_entry* entry;
    int err = archive_read_next_header(m_arch, &entry);

    if (err == ARCHIVE_EOF)
      return nullptr;

    if (err != ARCHIVE_OK)
      throw base::Exception("Error uncompressing extension\n%s",
                            archive_error_string(m_arch));

    return entry;
  }

  int copyDataTo(archive* out) {
    const void* buf;
    size_t size;
    int64_t offset;
    for (;;) {
      int err = archive_read_data_block(m_arch, &buf, &size, &offset);
      if (err == ARCHIVE_EOF)
        break;
      if (err != ARCHIVE_OK)
        return err;

      err = archive_write_data_block(out, buf, size, offset);
      if (err != ARCHIVE_OK) {
        throw base::Exception("Error writing data blocks\n%s (%d)",
                              archive_error_string(out), err);
        return err;
      }
    }
    return ARCHIVE_OK;
  }

private:
  base::FileHandle m_file;
  archive* m_arch;
  bool m_open;
};

class WriteArchive {
public:
  WriteArchive(const std::string& outputDir)
   : m_arch(nullptr)
   , m_open(false)
   , m_outputDir(outputDir) {
    m_arch = archive_write_disk_new();
    m_open = true;
  }

  ~WriteArchive() {
    if (m_arch) {
      if (m_open)
        archive_write_close(m_arch);
      archive_write_free(m_arch);
    }
  }

  void writeEntry(ReadArchive& in, archive_entry* entry) {
    const std::string origFn = archive_entry_pathname(entry);
    const std::string fullFn = base::join_path(m_outputDir, origFn);
    archive_entry_set_pathname(entry, fullFn.c_str());

    LOG("EXT: Uncompressing file <%s> to <%s>\n",
        origFn.c_str(), fullFn.c_str());

    int err = archive_write_header(m_arch, entry);
    if (err != ARCHIVE_OK)
      throw base::Exception("Error writing file into disk\n%s (%d)",
                            archive_error_string(m_arch), err);

    in.copyDataTo(m_arch);
    err = archive_write_finish_entry(m_arch);
    if (err != ARCHIVE_OK)
      throw base::Exception("Error saving the last part of a file entry in disk\n%s (%d)",
                            archive_error_string(m_arch), err);
  }

private:
  archive* m_arch;
  bool m_open;
  std::string m_outputDir;
};

} // anonymous namespace

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

void Extension::addTheme(const std::string& id, const std::string& path)
{
  m_themes[id] = path;
}

void Extension::addPalette(const std::string& id, const std::string& path)
{
  m_palettes[id] = path;
}

bool Extension::canBeDisabled() const
{
  return (m_isEnabled && !isCurrentTheme());
}

bool Extension::canBeUninstalled() const
{
  return (!m_isBuiltinExtension && !isCurrentTheme());
}

void Extension::enable(const bool state)
{
  // Do nothing
  if (m_isEnabled == state)
    return;

  set_config_bool("extensions", m_name.c_str(), state);
  flush_config_file();

  m_isEnabled = state;
  Enable(this, state);
}

void Extension::uninstall()
{
  if (!m_isInstalled)
    return;

  ASSERT(canBeUninstalled());
  if (!canBeUninstalled())
    return;

  TRACE("EXT: Uninstall extension '%s' from '%s'...\n",
        m_name.c_str(), m_path.c_str());

  // Remove all files inside the extension path
  uninstallFiles(m_path);
  ASSERT(!base::is_directory(m_path));

  m_isEnabled = false;
  m_isInstalled = false;
  Uninstall(this);
}

void Extension::uninstallFiles(const std::string& path)
{
  for (auto& item : base::list_files(path)) {
    std::string fn = base::join_path(path, item);
    if (base::is_file(fn)) {
      TRACE("EXT: Deleting file '%s'\n", fn.c_str());
      base::delete_file(fn);
    }
    else if (base::is_directory(fn)) {
      uninstallFiles(fn);
    }
  }

  TRACE("EXT: Deleting directory '%s'\n", path.c_str());
  base::remove_directory(path);
}

bool Extension::isCurrentTheme() const
{
  auto it = m_themes.find(Preferences::instance().theme.selected.defaultValue());
  return (it != m_themes.end());
}

Extensions::Extensions()
{
  // Create and get the user extensions directory
  {
    ResourceFinder rf2;
    rf2.includeUserDir("data/extensions/.");
    m_userExtensionsPath = rf2.getFirstOrCreateDefault();
    m_userExtensionsPath = base::normalize_path(m_userExtensionsPath);
    m_userExtensionsPath = base::get_file_path(m_userExtensionsPath);
    LOG("EXT: User extensions path '%s'\n", m_userExtensionsPath.c_str());
  }

  ResourceFinder rf;
  rf.includeDataDir("extensions");

  // Load extensions from data/ directory on all possible locations
  // (installed folder and user folder)
  while (rf.next()) {
    auto extensionsDir = rf.filename();

    if (base::is_directory(extensionsDir)) {
      for (auto fn : base::list_files(extensionsDir)) {
        const auto dir = base::join_path(extensionsDir, fn);
        if (!base::is_directory(dir))
          continue;

        const bool isBuiltinExtension =
          (m_userExtensionsPath != base::get_file_path(dir));

        auto fullFn = base::join_path(dir, "package.json");
        fullFn = base::normalize_path(fullFn);

        LOG("EXT: Loading extension '%s'...\n", fullFn.c_str());
        if (!base::is_file(fullFn)) {
          LOG("EXT: File '%s' not found\n", fullFn.c_str());
          continue;
        }

        try {
          loadExtension(dir, fullFn, isBuiltinExtension);
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
  for (auto ext : m_extensions) {
    auto it = ext->themes().find(themeId);
    if (it != ext->themes().end())
      return it->second;
  }
  return std::string();
}

ExtensionItems Extensions::palettes() const
{
  ExtensionItems palettes;
  for (auto ext : m_extensions)
    for (auto item : ext->palettes())
      palettes[item.first] = item.second;
  return palettes;
}

Extension* Extensions::installCompressedExtension(const std::string& zipFn)
{
  std::string dstExtensionPath =
    base::join_path(m_userExtensionsPath,
                    base::get_file_title(zipFn));

  ReadArchive in(zipFn);
  WriteArchive out(dstExtensionPath);

  archive_entry* entry;
  while ((entry = in.readEntry()) != nullptr)
    out.writeEntry(in, entry);

  Extension* extension = loadExtension(
    dstExtensionPath,
    base::join_path(dstExtensionPath, "package.json"),
    false);
  if (!extension)
    throw base::Exception("Error adding the new extension");

  // Generate signal
  NewExtension(extension);
  return extension;
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
                  // Extensions are enabled by default
                  get_config_bool("extensions", name.c_str(), true),
                  isBuiltinExtension));

  auto contributes = json["contributes"];
  if (contributes.is_object()) {
    // Themes
    auto themes = contributes["themes"];
    if (themes.is_array()) {
      for (const auto& theme : themes.get_array()) {
        std::string themeId = theme.at("id").get_string();
        std::string themePath = theme.at("path").get_string();

        // The path must be always relative to the extension
        themePath = base::join_path(path, themePath);

        LOG("EXT: New theme '%s' in '%s'\n",
            themeId.c_str(),
            themePath.c_str());

        extension->addTheme(themeId, themePath);
      }
    }

    // Palettes
    auto palettes = contributes["palettes"];
    if (palettes.is_array()) {
      for (const auto& palette : palettes.get_array()) {
        std::string palId = palette.at("id").get_string();
        std::string palPath = palette.at("path").get_string();

        // The path must be always relative to the extension
        palPath = base::join_path(path, palPath);

        LOG("EXT: New palette '%s' in '%s'\n",
            palId.c_str(),
            palPath.c_str());

        extension->addPalette(palId, palPath);
      }
    }
  }

  if (extension)
    m_extensions.push_back(extension.get());
  return extension.release();
}

} // namespace app
