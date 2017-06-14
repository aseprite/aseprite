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
#include "app/load_matrix.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "base/exception.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "base/unique_ptr.h"
#include "render/dithering_matrix.h"

#include "archive.h"
#include "archive_entry.h"
#include "json11.hpp"

#include <fstream>
#include <queue>
#include <sstream>
#include <string>

namespace app {

namespace {

const char* kPackageJson = "package.json";
const char* kAsepriteDefaultThemeExtensionName = "aseprite-theme";

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

  int copyDataTo(std::ostream& dst) {
    const void* buf;
    size_t size;
    int64_t offset;
    for (;;) {
      int err = archive_read_data_block(m_arch, &buf, &size, &offset);
      if (err == ARCHIVE_EOF)
        break;
      if (err != ARCHIVE_OK)
        return err;
      dst.write((const char*)buf, size);
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
  WriteArchive()
   : m_arch(nullptr)
   , m_open(false) {
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
};

} // anonymous namespace

const render::DitheringMatrix& Extension::DitheringMatrixInfo::matrix() const
{
  if (!m_matrix) {
    m_matrix = new render::DitheringMatrix;
    load_dithering_matrix_from_sprite(m_path, *m_matrix);
  }
  return *m_matrix;
}

void Extension::DitheringMatrixInfo::destroyMatrix()
{
  if (m_matrix)
    delete m_matrix;
}

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

Extension::~Extension()
{
  // Delete all matrices
  for (auto& it : m_ditheringMatrices)
    it.second.destroyMatrix();
}

void Extension::addTheme(const std::string& id, const std::string& path)
{
  m_themes[id] = path;
}

void Extension::addPalette(const std::string& id, const std::string& path)
{
  m_palettes[id] = path;
}

void Extension::addDitheringMatrix(const std::string& id,
                                   const std::string& path,
                                   const std::string& name)
{
  DitheringMatrixInfo info(path, name);
  m_ditheringMatrices[id] = info;
}

bool Extension::canBeDisabled() const
{
  return (m_isEnabled &&
          !isCurrentTheme() &&
          !isDefaultTheme());   // Default theme cannot be disabled or uninstalled
}

bool Extension::canBeUninstalled() const
{
  return (!m_isBuiltinExtension &&
          !isCurrentTheme() &&
          !isDefaultTheme());
}

void Extension::enable(const bool state)
{
  // Do nothing
  if (m_isEnabled == state)
    return;

  set_config_bool("extensions", m_name.c_str(), state);
  flush_config_file();

  m_isEnabled = state;
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
  auto it = m_themes.find(Preferences::instance().theme.selected());
  return (it != m_themes.end());
}

bool Extension::isDefaultTheme() const
{
  return (name() == kAsepriteDefaultThemeExtensionName);
}

Extensions::Extensions()
{
  // Create and get the user extensions directory
  {
    ResourceFinder rf2;
    rf2.includeUserDir("extensions/.");
    m_userExtensionsPath = rf2.getFirstOrCreateDefault();
    m_userExtensionsPath = base::normalize_path(m_userExtensionsPath);
    m_userExtensionsPath = base::get_file_path(m_userExtensionsPath);
    LOG("EXT: User extensions path '%s'\n", m_userExtensionsPath.c_str());
  }

  ResourceFinder rf;
  rf.includeUserDir("extensions");
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

        auto fullFn = base::join_path(dir, kPackageJson);
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
    if (!ext->isEnabled())      // Ignore disabled extensions
      continue;

    auto it = ext->themes().find(themeId);
    if (it != ext->themes().end())
      return it->second;
  }
  return std::string();
}

std::string Extensions::palettePath(const std::string& palId)
{
  for (auto ext : m_extensions) {
    if (!ext->isEnabled())      // Ignore disabled extensions
      continue;

    auto it = ext->palettes().find(palId);
    if (it != ext->palettes().end())
      return it->second;
  }
  return std::string();
}

ExtensionItems Extensions::palettes() const
{
  ExtensionItems palettes;
  for (auto ext : m_extensions) {
    if (!ext->isEnabled())      // Ignore disabled themes
      continue;

    for (auto item : ext->palettes())
      palettes[item.first] = item.second;
  }
  return palettes;
}

const render::DitheringMatrix* Extensions::ditheringMatrix(const std::string& matrixId)
{
  for (auto ext : m_extensions) {
    if (!ext->isEnabled())      // Ignore disabled themes
      continue;

    auto it = ext->m_ditheringMatrices.find(matrixId);
    if (it != ext->m_ditheringMatrices.end())
      return &it->second.matrix();
  }
  return nullptr;
}

std::vector<Extension::DitheringMatrixInfo> Extensions::ditheringMatrices()
{
  std::vector<Extension::DitheringMatrixInfo> result;
  for (auto ext : m_extensions) {
    if (!ext->isEnabled())      // Ignore disabled themes
      continue;

    for (auto it : ext->m_ditheringMatrices)
      result.push_back(it.second);
  }
  return result;
}

void Extensions::enableExtension(Extension* extension, const bool state)
{
  extension->enable(state);
  generateExtensionSignals(extension);
}

void Extensions::uninstallExtension(Extension* extension)
{
  extension->uninstall();
  generateExtensionSignals(extension);
}

Extension* Extensions::installCompressedExtension(const std::string& zipFn)
{
  std::string dstExtensionPath =
    base::join_path(m_userExtensionsPath,
                    base::get_file_title(zipFn));

  // First of all we read the package.json file inside the .zip to
  // know 1) the extension name, 2) that the .json file can be parsed
  // correctly, 3) the final destination directory.
  std::string commonPath;
  {
    ReadArchive in(zipFn);
    archive_entry* entry;
    while ((entry = in.readEntry()) != nullptr) {
      const std::string entryFn = archive_entry_pathname(entry);
      if (base::get_file_name(entryFn) != kPackageJson)
        continue;

      commonPath = base::get_file_path(entryFn);
      if (!commonPath.empty() &&
          entryFn.size() > commonPath.size())
        commonPath.push_back(entryFn[commonPath.size()]);

      std::stringstream out;
      in.copyDataTo(out);

      std::string err;
      auto json = json11::Json::parse(out.str(), err);
      if (err.empty()) {
        auto name = json["name"].string_value();
        dstExtensionPath = base::join_path(m_userExtensionsPath, name);
      }
      break;
    }
  }

  // Uncompress zipFn in dstExtensionPath
  {
    ReadArchive in(zipFn);
    WriteArchive out;

    archive_entry* entry;
    while ((entry = in.readEntry()) != nullptr) {
      // Fix the entry filename to write the file in the disk
      std::string fn = archive_entry_pathname(entry);

      LOG("EXT: Original filename in zip <%s>...\n", fn.c_str());

      if (!commonPath.empty()) {
        // Check mismatch with package.json common path
        if (fn.compare(0, commonPath.size(), commonPath) != 0)
          continue;

        fn.erase(0, commonPath.size());
        if (fn.empty())
          continue;
      }

      const std::string fullFn = base::join_path(dstExtensionPath, fn);
      archive_entry_set_pathname(entry, fullFn.c_str());

      LOG("EXT: Uncompressing file <%s> to <%s>\n",
          fn.c_str(), fullFn.c_str());

      out.writeEntry(in, entry);
    }
  }

  Extension* extension = loadExtension(
    dstExtensionPath,
    base::join_path(dstExtensionPath, kPackageJson),
    false);
  if (!extension)
    throw base::Exception("Error adding the new extension");

  // Generate signals
  NewExtension(extension);
  generateExtensionSignals(extension);

  return extension;
}

Extension* Extensions::loadExtension(const std::string& path,
                                     const std::string& fullPackageFilename,
                                     const bool isBuiltinExtension)
{
  json11::Json json;
  {
    std::string jsonText, line;
    std::ifstream in(FSTREAM_PATH(fullPackageFilename), std::ifstream::binary);
    while (std::getline(in, line)) {
      jsonText += line;
      jsonText.push_back('\n');
    }

    std::string err;
    json = json11::Json::parse(jsonText, err);
    if (!err.empty())
      throw base::Exception("Error parsing JSON file: %s\n",
                            err.c_str());
  }
  auto name = json["name"].string_value();
  auto displayName = json["displayName"].string_value();

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
      for (const auto& theme : themes.array_items()) {
        std::string themeId = theme["id"].string_value();
        std::string themePath = theme["path"].string_value();

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
      for (const auto& palette : palettes.array_items()) {
        std::string palId = palette["id"].string_value();
        std::string palPath = palette["path"].string_value();

        // The path must be always relative to the extension
        palPath = base::join_path(path, palPath);

        LOG("EXT: New palette '%s' in '%s'\n",
            palId.c_str(),
            palPath.c_str());

        extension->addPalette(palId, palPath);
      }
    }

    // Dithering matrices
    auto ditheringMatrices = contributes["ditheringMatrices"];
    if (ditheringMatrices.is_array()) {
      for (const auto& ditheringMatrix : ditheringMatrices.array_items()) {
        std::string matId = ditheringMatrix["id"].string_value();
        std::string matPath = ditheringMatrix["path"].string_value();
        std::string matName = ditheringMatrix["name"].string_value();
        if (matName.empty())
          matName = matId;

        // The path must be always relative to the extension
        matPath = base::join_path(path, matPath);

        LOG("EXT: New dithering matrix '%s' in '%s'\n",
            matId.c_str(),
            matPath.c_str());

        extension->addDitheringMatrix(matId, matPath, matName);
      }
    }
  }

  if (extension)
    m_extensions.push_back(extension.get());
  return extension.release();
}

void Extensions::generateExtensionSignals(Extension* extension)
{
  if (extension->hasThemes()) ThemesChange(extension);
  if (extension->hasPalettes()) PalettesChange(extension);
  if (extension->hasDitheringMatrices()) DitheringMatricesChange(extension);
}

} // namespace app
