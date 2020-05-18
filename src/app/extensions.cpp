// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/extensions.h"

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/ini_file.h"
#include "app/load_matrix.h"
#include "app/pref/preferences.h"
#include "app/resource_finder.h"
#include "base/exception.h"
#include "base/file_content.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "render/dithering_matrix.h"

#ifdef ENABLE_SCRIPTING
  #include "app/script/engine.h"
  #include "app/script/luacpp.h"
#endif

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
const char* kInfoJson = "__info.json";
const char* kPrefLua = "__pref.lua";
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

void read_json_file(const std::string& path, json11::Json& json)
{
  std::string jsonText, line;
  std::ifstream in(FSTREAM_PATH(path), std::ifstream::binary);
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

void write_json_file(const std::string& path, const json11::Json& json)
{
  std::string text;
  json.dump(text);
  std::ofstream out(FSTREAM_PATH(path), std::ifstream::binary);
  out.write(text.c_str(), text.size());
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////
// Extension

Extension::DitheringMatrixInfo::DitheringMatrixInfo()
{
}

Extension::DitheringMatrixInfo::DitheringMatrixInfo(const std::string& path,
                                                    const std::string& name)
  : m_path(path)
  , m_name(name)
{
}

const render::DitheringMatrix& Extension::DitheringMatrixInfo::matrix() const
{
  if (!m_loaded) {
    m_loaded = true;
    load_dithering_matrix_from_sprite(m_path, m_matrix);
  }
  return m_matrix;
}

Extension::Extension(const std::string& path,
                     const std::string& name,
                     const std::string& version,
                     const std::string& displayName,
                     const bool isEnabled,
                     const bool isBuiltinExtension)
  : m_path(path)
  , m_name(name)
  , m_version(version)
  , m_displayName(displayName)
  , m_category(Category::None)
  , m_isEnabled(isEnabled)
  , m_isInstalled(true)
  , m_isBuiltinExtension(isBuiltinExtension)
{
#ifdef ENABLE_SCRIPTING
  m_plugin.pluginRef = LUA_REFNIL;
#endif
}

Extension::~Extension()
{
}

void Extension::executeInitActions()
{
#ifdef ENABLE_SCRIPTING
  if (isEnabled() && hasScripts())
    initScripts();
#endif
}

void Extension::executeExitActions()
{
#ifdef ENABLE_SCRIPTING
  if (isEnabled() && hasScripts())
    exitScripts();
#endif // ENABLE_SCRIPTING
}

void Extension::addLanguage(const std::string& id, const std::string& path)
{
  m_languages[id] = path;
  updateCategory(Category::Languages);
}

void Extension::addTheme(const std::string& id, const std::string& path)
{
  m_themes[id] = path;
  updateCategory(Category::Themes);
}

void Extension::addPalette(const std::string& id, const std::string& path)
{
  m_palettes[id] = path;
  updateCategory(Category::Palettes);
}

void Extension::addDitheringMatrix(const std::string& id,
                                   const std::string& path,
                                   const std::string& name)
{
  DitheringMatrixInfo info(path, name);
  m_ditheringMatrices[id] = std::move(info);
  updateCategory(Category::DitheringMatrices);
}

#ifdef ENABLE_SCRIPTING

void Extension::addCommand(const std::string& id)
{
  PluginItem item;
  item.type = PluginItem::Command;
  item.id = id;
  m_plugin.items.push_back(item);
}

void Extension::removeCommand(const std::string& id)
{
  for (auto it=m_plugin.items.begin(); it != m_plugin.items.end(); ) {
    if (it->type == PluginItem::Command &&
        it->id == id) {
      it = m_plugin.items.erase(it);
    }
    else {
      ++it;
    }
  }
}

#endif

bool Extension::canBeDisabled() const
{
  return (m_isEnabled &&
          //!isCurrentTheme() &&
          !isDefaultTheme()); // Default theme cannot be disabled or uninstalled
}

bool Extension::canBeUninstalled() const
{
  return (!m_isBuiltinExtension &&
          // We can uninstall the current theme (e.g. to upgrade it)
          //!isCurrentTheme() &&
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

#ifdef ENABLE_SCRIPTING
  if (hasScripts()) {
    if (m_isEnabled) {
      initScripts();
    }
    else {
      exitScripts();
    }
  }
#endif // ENABLE_SCRIPTING
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
#if 1 // Read the list of files to be uninstalled from __info.json file

  std::string infoFn = base::join_path(path, kInfoJson);
  if (!base::is_file(infoFn))
    throw base::Exception("Cannot remove extension, '%s' file doesn't exist",
                          infoFn.c_str());

  json11::Json json;
  read_json_file(infoFn, json);

  base::paths installedDirs;

  for (const auto& value : json["installedFiles"].array_items()) {
    std::string fn = base::join_path(path, value.string_value());
    if (base::is_file(fn)) {
      TRACE("EXT: Deleting file '%s'\n", fn.c_str());
      base::delete_file(fn);
    }
    else if (base::is_directory(fn)) {
      installedDirs.push_back(fn);
    }
  }

  // Delete __pref.lua file
  {
    std::string fn = base::join_path(path, kPrefLua);
    if (base::is_file(fn))
      base::delete_file(fn);
  }

  std::sort(installedDirs.begin(),
            installedDirs.end(),
            [](const std::string& a,
               const std::string& b) {
              return b.size() < a.size();
            });

  for (const auto& dir : installedDirs) {
    TRACE("EXT: Deleting directory '%s'\n", dir.c_str());
    base::remove_directory(dir);
  }

  // Delete __info.json file if it does exist (e.g. maybe the
  // "installedFiles" list included the __info.json so the file was
  // already deleted, this can happen if the .json file was modified
  // by hand/the user)
  if (base::is_file(infoFn)) {
    TRACE("EXT: Deleting file '%s'\n", infoFn.c_str());
    base::delete_file(infoFn);
  }

  TRACE("EXT: Deleting extension directory '%s'\n", path.c_str());
  base::remove_directory(path);

#else // The following code delete the whole "path",
      // we prefer the __info.json approach.

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

#endif
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

void Extension::updateCategory(const Category newCategory)
{
  if (m_category == Category::None)
    m_category = newCategory;
  else if (m_category != newCategory)
    m_category = Category::Multiple;
}

#ifdef ENABLE_SCRIPTING

// TODO move this to app/script/tableutils.h
static void serialize_table(lua_State* L, int idx, std::string& result)
{
  bool first = true;

  result.push_back('{');

  idx = lua_absindex(L, idx);
  lua_pushnil(L);
  while (lua_next(L, idx) != 0) {
    if (first) {
      first = false;
    }
    else {
      result.push_back(',');
    }

    // Save key
    if (lua_type(L, -2) == LUA_TSTRING) {
      if (const char* k = lua_tostring(L, -2)) {
        result += k;
        result.push_back('=');
      }
    }

    // Save value
    switch (lua_type(L, -1)) {
      case LUA_TNIL:
      default:
        result += "nil";
        break;
      case LUA_TBOOLEAN:
        if (lua_toboolean(L, -1))
          result += "true";
        else
          result += "false";
        break;
      case LUA_TNUMBER:
        result += lua_tostring(L, -1);
        break;
      case LUA_TSTRING:
        result.push_back('\"');
        if (const char* p = lua_tostring(L, -1)) {
          for (; *p; ++p) {
            switch (*p) {
              case '\"':
                result.push_back('\\');
                result.push_back('\"');
                break;
              case '\\':
                result.push_back('\\');
                result.push_back('\\');
                break;
              case '\t':
                result.push_back('\\');
                result.push_back('t');
                break;
              case '\r':
                result.push_back('\\');
                result.push_back('n');
                break;
              case '\n':
                result.push_back('\\');
                result.push_back('n');
                break;
              default:
                result.push_back(*p);
                break;
            }
          }
        }
        result.push_back('\"');
        break;
      case LUA_TTABLE:
        serialize_table(L, -1, result);
        break;
    }
    lua_pop(L, 1);
  }

  result.push_back('}');
}

Extension::ScriptItem::ScriptItem(const std::string& fn)
  : fn(fn)
  , exitFunctionRef(LUA_REFNIL)
{
}

void Extension::initScripts()
{
  script::Engine* engine = App::instance()->scriptEngine();
  lua_State* L = engine->luaState();

  // Put a new "plugin" object for init()/exit() functions
  script::push_plugin(L, this);
  m_plugin.pluginRef = luaL_ref(L, LUA_REGISTRYINDEX);

  // Read plugin.preferences value
  {
    std::string fn = base::join_path(m_path, kPrefLua);
    if (base::is_file(fn)) {
      lua_rawgeti(L, LUA_REGISTRYINDEX, m_plugin.pluginRef);
      if (luaL_loadfile(L, fn.c_str()) == LUA_OK) {
        if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
          lua_setfield(L, -2, "preferences");
        }
        else {
          const char* s = lua_tostring(L, -1);
          if (s) {
            Console().printf("%s\n", s);
          }
        }
        lua_pop(L, 1);
      }
      else {
        lua_pop(L, 1);
      }
    }
  }

  for (auto& script : m_plugin.scripts) {
    // Reset global init()/exit() functions
    engine->evalCode("init=nil exit=nil");

    // Eval the code of the script (it should define an init() and an exit() function)
    engine->evalFile(script.fn);

    if (lua_getglobal(L, "exit") == LUA_TFUNCTION) {
      // Save a reference to the exit() function of this script
      script.exitFunctionRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else {
      lua_pop(L, 1);
    }

    // Call the init() function of thi sscript with a Plugin object as first parameter
    if (lua_getglobal(L, "init") == LUA_TFUNCTION) {
      // Call init(plugin)
      lua_rawgeti(L, LUA_REGISTRYINDEX, m_plugin.pluginRef);
      lua_pcall(L, 1, 1, 0);
      lua_pop(L, 1);
    }
    else {
      lua_pop(L, 1);
    }
  }
}

void Extension::exitScripts()
{
  script::Engine* engine = App::instance()->scriptEngine();
  lua_State* L = engine->luaState();

  // Call the exit() function of each script
  for (auto& script : m_plugin.scripts) {
    if (script.exitFunctionRef != LUA_REFNIL) {
      // Get the exit() function, the "plugin" object, and call exit(plugin)
      lua_rawgeti(L, LUA_REGISTRYINDEX, script.exitFunctionRef);
      lua_rawgeti(L, LUA_REGISTRYINDEX, m_plugin.pluginRef);
      lua_pcall(L, 1, 0, 0);

      luaL_unref(L, LUA_REGISTRYINDEX, script.exitFunctionRef);
      script.exitFunctionRef = LUA_REFNIL;
    }
  }

  // Save the plugin preferences object
  if (m_plugin.pluginRef != LUA_REFNIL) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, m_plugin.pluginRef);
    lua_getfield(L, -1, "preferences");

    lua_pushnil(L); // Push a nil key, to ask for the first element of the table
    bool hasPreferences = (lua_next(L, -2) != 0);
    if (hasPreferences)
      lua_pop(L, 2);        // Remove the value and the key

    if (hasPreferences) {
      std::string result = "return ";
      serialize_table(L, -1, result);
      base::write_file_content(
        base::join_path(m_path, kPrefLua),
        (const uint8_t*)result.c_str(), result.size());
    }
    lua_pop(L, 2);          // Pop preferences table and plugin

    luaL_unref(L, LUA_REGISTRYINDEX, m_plugin.pluginRef);
    m_plugin.pluginRef = LUA_REFNIL;
  }

  // Remove plugin items automatically
  for (const auto& item : m_plugin.items) {
    switch (item.type) {
      case PluginItem::Command: {
        auto cmds = Commands::instance();
        auto cmd = cmds->byId(item.id.c_str());
        ASSERT(cmd);

        if (cmd) {
#ifdef ENABLE_UI
          // TODO use a signal
          AppMenus::instance()->removeMenuItemFromGroup(cmd);
#endif // ENABLE_UI

          cmds->remove(cmd);

          // This will call ~PluginCommand() and unref the command
          // onclick callback.
          delete cmd;
        }
        break;
      }
    }
  }
  m_plugin.items.clear();
}

void Extension::addScript(const std::string& fn)
{
  m_plugin.scripts.push_back(ScriptItem(fn));
  updateCategory(Category::Scripts);
}

#endif // ENABLE_SCRIPTING

//////////////////////////////////////////////////////////////////////
// Extensions

Extensions::Extensions()
{
  // Create and get the user extensions directory
  {
    ResourceFinder rf2;
    rf2.includeUserDir("extensions/.");
    m_userExtensionsPath = rf2.getFirstOrCreateDefault();
    m_userExtensionsPath = base::normalize_path(m_userExtensionsPath);
    if (!m_userExtensionsPath.empty() &&
        m_userExtensionsPath.back() == '.') {
      m_userExtensionsPath = base::get_file_path(m_userExtensionsPath);
    }
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

void Extensions::executeInitActions()
{
  for (auto& ext : m_extensions)
    ext->executeInitActions();

  ScriptsChange(nullptr);
}

void Extensions::executeExitActions()
{
  for (auto& ext : m_extensions)
    ext->executeExitActions();

  ScriptsChange(nullptr);
}

std::string Extensions::languagePath(const std::string& langId)
{
  for (auto ext : m_extensions) {
    if (!ext->isEnabled())      // Ignore disabled extensions
      continue;

    auto it = ext->languages().find(langId);
    if (it != ext->languages().end())
      return it->second;
  }
  return std::string();
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

  auto it = std::find(m_extensions.begin(),
                      m_extensions.end(), extension);
  ASSERT(it != m_extensions.end());
  if (it != m_extensions.end())
    m_extensions.erase(it);

  delete extension;
}

ExtensionInfo Extensions::getCompressedExtensionInfo(const std::string& zipFn)
{
  ExtensionInfo info;
  info.dstPath =
    base::join_path(m_userExtensionsPath,
                    base::get_file_title(zipFn));

  // First of all we read the package.json file inside the .zip to
  // know 1) the extension name, 2) that the .json file can be parsed
  // correctly, 3) the final destination directory.
  ReadArchive in(zipFn);
  archive_entry* entry;
  while ((entry = in.readEntry()) != nullptr) {
    const char* entryFnPtr = archive_entry_pathname(entry);

    // This can happen, e.g. if the file contains "!" + unicode chars
    // (maybe a bug in archive library?)
    // TODO try to find the real cause of this on libarchive
    if (!entryFnPtr)
      continue;

    const std::string entryFn = entryFnPtr;
    if (base::get_file_name(entryFn) != kPackageJson)
      continue;

    info.commonPath = base::get_file_path(entryFn);
    if (!info.commonPath.empty() &&
        entryFn.size() > info.commonPath.size()) {
      info.commonPath.push_back(entryFn[info.commonPath.size()]);
    }

    std::stringstream out;
    in.copyDataTo(out);

    std::string err;
    auto json = json11::Json::parse(out.str(), err);
    if (err.empty()) {
      info.name = json["name"].string_value();
      info.version = json["version"].string_value();
      info.dstPath = base::join_path(m_userExtensionsPath, info.name);
    }
    break;
  }
  return info;
}

Extension* Extensions::installCompressedExtension(const std::string& zipFn,
                                                  const ExtensionInfo& info)
{
  base::paths installedFiles;

  // Uncompress zipFn in info.dstPath
  {
    ReadArchive in(zipFn);
    WriteArchive out;

    archive_entry* entry;
    while ((entry = in.readEntry()) != nullptr) {
      const char* entryFnPtr = archive_entry_pathname(entry);
      if (!entryFnPtr)
        continue;

      // Fix the entry filename to write the file in the disk
      std::string fn = entryFnPtr;

      LOG("EXT: Original filename in zip <%s>...\n", fn.c_str());

      // Do not install __info.json file if it's inside the .zip as
      // some users are distirbuting extensions with the __info.json
      // file inside.
      if (base::string_to_lower(base::get_file_name(fn)) == kInfoJson) {
        LOG("EXT: Ignoring <%s>...\n", fn.c_str());
        continue;
      }

      if (!info.commonPath.empty()) {
        // Check mismatch with package.json common path
        if (fn.compare(0, info.commonPath.size(), info.commonPath) != 0)
          continue;

        fn.erase(0, info.commonPath.size());
        if (fn.empty())
          continue;
      }

      installedFiles.push_back(fn);

      const std::string fullFn = base::join_path(info.dstPath, fn);
#if _WIN32
      archive_entry_copy_pathname_w(entry, base::from_utf8(fullFn).c_str());
#else
      archive_entry_set_pathname(entry, fullFn.c_str());
#endif

      LOG("EXT: Uncompressing file <%s> to <%s>\n",
          fn.c_str(), fullFn.c_str());

      out.writeEntry(in, entry);
    }
  }

  // Save the list of installed files in "__info.json" file
  {
    json11::Json::object obj;
    obj["installedFiles"] = json11::Json(installedFiles);
    json11::Json json(obj);

    const std::string fullFn = base::join_path(info.dstPath, kInfoJson);
    LOG("EXT: Saving list of installed files in <%s>\n", fullFn.c_str());
    write_json_file(fullFn, json);
  }

  // Load the extension
  Extension* extension = loadExtension(
    info.dstPath,
    base::join_path(info.dstPath, kPackageJson),
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
  read_json_file(fullPackageFilename, json);
  auto name = json["name"].string_value();
  auto version = json["version"].string_value();
  auto displayName = json["displayName"].string_value();

  LOG("EXT: Extension '%s' loaded\n", name.c_str());

  std::unique_ptr<Extension> extension(
    new Extension(path,
                  name,
                  version,
                  displayName,
                  // Extensions are enabled by default
                  get_config_bool("extensions", name.c_str(), true),
                  isBuiltinExtension));

  auto contributes = json["contributes"];
  if (contributes.is_object()) {
    // Languages
    auto languages = contributes["languages"];
    if (languages.is_array()) {
      for (const auto& lang : languages.array_items()) {
        std::string langId = lang["id"].string_value();
        std::string langPath = lang["path"].string_value();

        // The path must be always relative to the extension
        langPath = base::join_path(path, langPath);

        LOG("EXT: New language '%s' in '%s'\n",
            langId.c_str(),
            langPath.c_str());

        extension->addLanguage(langId, langPath);
      }
    }

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

#ifdef ENABLE_SCRIPTING
    // Scripts
    auto scripts = contributes["scripts"];
    if (scripts.is_array()) {
      for (const auto& script : scripts.array_items()) {
        std::string scriptPath = script["path"].string_value();
        if (scriptPath.empty())
          continue;

        // The path must be always relative to the extension
        scriptPath = base::join_path(path, scriptPath);

        LOG("EXT: New script '%s'\n", scriptPath.c_str());

        extension->addScript(scriptPath);
      }
    }
    // Simple version of packages.json with {... "scripts": "file.lua" ...}
    else if (scripts.is_string() &&
             !scripts.string_value().empty()) {
      std::string scriptPath = scripts.string_value();

      // The path must be always relative to the extension
      scriptPath = base::join_path(path, scriptPath);

      LOG("EXT: New script '%s'\n", scriptPath.c_str());

      extension->addScript(scriptPath);
    }
#endif // ENABLE_SCRIPTING
  }

  if (extension)
    m_extensions.push_back(extension.get());
  return extension.release();
}

void Extensions::generateExtensionSignals(Extension* extension)
{
  if (extension->hasLanguages()) LanguagesChange(extension);
  if (extension->hasThemes()) ThemesChange(extension);
  if (extension->hasPalettes()) PalettesChange(extension);
  if (extension->hasDitheringMatrices()) DitheringMatricesChange(extension);
#ifdef ENABLE_SCRIPTING
  if (extension->hasScripts()) ScriptsChange(extension);
#endif
}

} // namespace app
