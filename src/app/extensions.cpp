// Aseprite
// Copyright (C) 2020-2025  Igara Studio S.A.
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
#include "context.h"
#include "dio/detect_format.h"
#include "doc/file/act_file.h"
#include "doc_undo.h"
#include "file/file.h"
#include "file/file_format.h"
#include "file/file_formats_manager.h"
#include "render/dithering_matrix.h"
#include "ui/system.h"
#include "ui/widget.h"

#if ENABLE_SENTRY
  #include "app/sentry_wrapper.h"
#endif

#ifdef ENABLE_SCRIPTING
  #include "app/script/engine.h"
  #include "app/script/luacpp.h"
  #include "app/script/require.h"
#endif

#include "archive.h"
#include "archive_entry.h"
#include "doc.h"
#include "json11.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <string>

#include "base/log.h"

#ifdef ENABLE_SCRIPTING
  #include "script/docobj.h"
  #include "script/security.h"
#endif

namespace app {

const char* Extension::kAsepriteDefaultThemeExtensionName = "aseprite-theme";
const char* Extension::kAsepriteDefaultThemeId = "default";

namespace {

constexpr const char* kPackageJson = "package.json";
constexpr const char* kInfoJson = "__info.json";
constexpr const char* kPrefLua = "__pref.lua";

class ReadArchive {
public:
  ReadArchive(const std::string& filename) : m_arch(nullptr), m_open(false)
  {
    m_arch = archive_read_new();
    archive_read_support_format_zip(m_arch);

    m_file = base::open_file(filename, "rb");
    if (!m_file)
      throw base::Exception("Error loading file %s", filename.c_str());

    if (int err = archive_read_open_FILE(m_arch, m_file.get()))
      throw base::Exception("Error uncompressing extension\n%s (%d)",
                            archive_error_string(m_arch),
                            err);

    m_open = true;
  }

  ~ReadArchive()
  {
    if (m_arch) {
      if (m_open)
        archive_read_close(m_arch);
      archive_read_free(m_arch);
    }
  }

  archive_entry* readEntry() const
  {
    archive_entry* entry;
    int err = archive_read_next_header(m_arch, &entry);

    if (err == ARCHIVE_EOF)
      return nullptr;

    if (err != ARCHIVE_OK)
      throw base::Exception("Error uncompressing extension\n%s", archive_error_string(m_arch));

    return entry;
  }

  int copyDataTo(archive* out)
  {
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
      if (err != ARCHIVE_OK)
        throw base::Exception("Error writing data blocks\n%s (%d)", archive_error_string(out), err);
    }
    return ARCHIVE_OK;
  }

  int copyDataTo(std::ostream& dst) const
  {
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
  WriteArchive() : m_arch(nullptr), m_open(false)
  {
    m_arch = archive_write_disk_new();
    m_open = true;
  }

  ~WriteArchive()
  {
    if (m_arch) {
      if (m_open)
        archive_write_close(m_arch);
      archive_write_free(m_arch);
    }
  }

  void writeEntry(ReadArchive& in, archive_entry* entry) const
  {
    int err = archive_write_header(m_arch, entry);
    if (err != ARCHIVE_OK)
      throw base::Exception("Error writing file into disk\n%s (%d)",
                            archive_error_string(m_arch),
                            err);

    in.copyDataTo(m_arch);
    err = archive_write_finish_entry(m_arch);
    if (err != ARCHIVE_OK)
      throw base::Exception("Error saving the last part of a file entry in disk\n%s (%d)",
                            archive_error_string(m_arch),
                            err);
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
    throw base::Exception("Error parsing JSON file: %s\n", err.c_str());
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

Extension::DitheringMatrixInfo::DitheringMatrixInfo(const std::string& path,
                                                    const std::string& name)
  : m_path(path)
  , m_name(name)
{
}

const render::DitheringMatrix& Extension::DitheringMatrixInfo::matrix() const
{
  if (!m_loaded) {
    load_dithering_matrix_from_sprite(m_path, m_matrix);
    m_loaded = true;
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

void Extension::addKeys(const std::string& id, const std::string& path)
{
  if (!m_isBuiltinExtension && !base::is_file(path))
    return;

  m_keys[id] = path;
  updateCategory(Category::Keys);
}

void Extension::addLanguage(const std::string& id,
                            const std::string& path,
                            const std::string& displayName)
{
  if (!m_isBuiltinExtension && !base::is_file(path))
    return;

  m_languages[id] = LangInfo(id, path, displayName);
  updateCategory(Category::Languages);
}

void Extension::addTheme(const std::string& id, const std::string& path, const std::string& variant)
{
  if (id == kAsepriteDefaultThemeId && !isDefaultTheme())
    return;
  m_themes[id] = ThemeInfo(path, variant);
  updateCategory(Category::Themes);
}

void Extension::addPalette(const std::string& id, const std::string& path)
{
  if (!m_isBuiltinExtension && !base::is_file(path))
    return;

  m_palettes[id] = path;
  updateCategory(Category::Palettes);
}

void Extension::addDitheringMatrix(const std::string& id,
                                   const std::string& path,
                                   const std::string& name)
{
  if (!m_isBuiltinExtension && !base::is_file(path))
    return;

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
  for (auto it = m_plugin.items.begin(); it != m_plugin.items.end();) {
    if (it->type == PluginItem::Command && it->id == id) {
      it = m_plugin.items.erase(it);
    }
    else {
      ++it;
    }
  }
}

void Extension::addMenuGroup(const std::string& id)
{
  PluginItem item;
  item.type = PluginItem::MenuGroup;
  item.id = id;
  m_plugin.items.push_back(item);
}

void Extension::removeMenuGroup(const std::string& id)
{
  for (auto it = m_plugin.items.begin(); it != m_plugin.items.end();) {
    if (it->type == PluginItem::MenuGroup && it->id == id) {
      it = m_plugin.items.erase(it);
    }
    else {
      ++it;
    }
  }
}

void Extension::addMenuSeparator(ui::Widget* widget)
{
  PluginItem item;
  item.type = PluginItem::MenuSeparator;
  item.widget = widget;
  m_plugin.items.push_back(item);
}

class ExtensionCustomFormat : public FileFormat {
public:
  explicit ExtensionCustomFormat(const Extension::CustomFormatDefinition& formatDefinition)
    : m_definition(formatDefinition)
  {
    m_dioFormat = dio::register_custom_format();

    for (const auto& ext : formatDefinition.extensions) {
      if (!dio::register_custom_format_extension(ext, m_dioFormat))
        throw base::Exception("Extension %s is already registered to a file format", ext.c_str());
    }

    if (formatDefinition.flags > 0)
      m_flags = formatDefinition.flags;
    else // TODO: What should the defaults be?
      m_flags = FILE_SUPPORT_RGB | FILE_SUPPORT_RGBA | FILE_SUPPORT_GRAY | FILE_SUPPORT_GRAYA |
                FILE_SUPPORT_INDEXED;

    // Account for the user setting only the alpha variants of RGB & GRAYSCALE
    if (m_flags & FILE_SUPPORT_RGBA && !(m_flags & FILE_SUPPORT_RGB))
      m_flags |= FILE_SUPPORT_RGB;

    if (m_flags & FILE_SUPPORT_GRAYA && !(m_flags & FILE_SUPPORT_GRAY))
      m_flags |= FILE_SUPPORT_GRAY;

    // Add the load/save support flags since we don't expose those to Lua
    if (formatDefinition.onloadRef > 0)
      m_flags |= FILE_SUPPORT_LOAD;

    if (formatDefinition.onsaveRef > 0)
      m_flags |= FILE_SUPPORT_SAVE;
  }

  ~ExtensionCustomFormat() override
  {
    for (const auto& ext : m_definition.extensions)
      dio::unregister_custom_format_extension(ext, m_dioFormat);
  }

protected:
  const char* onGetName() const override { return m_definition.name.c_str(); };

  void onGetExtensions(base::paths& exts) const override
  {
    for (const auto& ext : m_definition.extensions)
      exts.push_back(ext);
  }

  dio::FileFormat onGetDioFormat() const override { return m_dioFormat; }

  int onGetFlags() const override { return m_flags; };

  void revertDocState() const
  {
    const auto& docs = App::instance()->context()->documents();

    // Script removed a bunch of documents for some reason?
    if (docs.size() < m_originalDocs.size())
      return;

    std::vector<Doc*> docsToRemove;
    std::copy_if(docs.begin(), docs.end(), std::back_inserter(docsToRemove), [&](const Doc* arg) {
      return (std::find(m_originalDocs.begin(), m_originalDocs.end(), arg) == m_originalDocs.end());
    });

    if (docsToRemove.empty())
      return;

    for (auto* doc : docsToRemove) {
      App::instance()->context()->documents().remove(doc);
      delete doc;
    }
    App::instance()->context()->setActiveDocument(m_originalActiveDoc);
  }

  bool onLoad(FileOp* fop) override
  {
    script::Engine* engine = App::instance()->scriptEngine();
    lua_State* L = engine->luaState();

    lua_rawgeti(L, LUA_REGISTRYINDEX, m_definition.onloadRef);
    lua_pushstring(L, fop->filename().c_str());

    lua_pushcclosure(L, script::get_original_io_open(), 0);
    lua_pushstring(L, fop->filename().c_str());
    lua_pushstring(L, m_definition.binary ? "rb" : "r");

    ASSERT(m_originalDocs.empty());
    m_originalDocs.clear();
    m_originalActiveDoc = App::instance()->context()->activeDocument();
    for (auto* doc : App::instance()->context()->documents())
      m_originalDocs.push_back(doc);

    if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
      fop->setError("Failed to open file '%s' for reading, %s\n",
                    fop->filename().c_str(),
                    lua_tostring(L, -1));
      revertDocState();
      return false;
    }

    lua_pushvalue(L, -1);
    const int fileHandle = luaL_ref(L, LUA_REGISTRYINDEX);

    const auto call = lua_pcall(L, 2, 1, 0);
    closeFileHandle(L, fileHandle);

    if (call != LUA_OK) {
      if (const char* s = lua_tostring(L, -1))
        fop->setError(s);
      else
        fop->setError("Format error");
      revertDocState();
      return false;
    }

    const auto* sprite = script::may_get_docobj<Sprite>(L, -1);
    if (!sprite) {
      fop->setError("Invalid sprite returned by script");
      revertDocState();
      return false;
    }

    auto* doc = static_cast<Doc*>(sprite->document());
    doc->resetUndoHistory();
    doc->setFilename(fop->filename());
    doc->markAsSaved();
    fop->setDocument(doc);
    return true;
  };

  #ifdef ENABLE_SAVE
  bool onSave(FileOp* fop) override
  {
    const auto* sprite = fop->document()->sprite();

    script::Engine* engine = App::instance()->scriptEngine();
    lua_State* L = engine->luaState();

    lua_rawgeti(L, LUA_REGISTRYINDEX, m_definition.onsaveRef);
    lua_pushstring(L, fop->filename().c_str());

    lua_pushcclosure(L, script::get_original_io_open(), 0);
    lua_pushstring(L, fop->filename().c_str());
    lua_pushstring(L, m_definition.binary ? "wb" : "w");

    if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
      Console().printf("Failed to open file for writing");
      return false;
    }
    lua_pushvalue(L, -1);
    const int fileHandle = luaL_ref(L, LUA_REGISTRYINDEX);

    script::push_docobj(L, sprite);

    const auto& roi = fop->roi();

    // Creating push options table.
    lua_newtable(L);
    script::push_value_to_lua<gfx::Size>(L, roi.fileCanvasSize());
    lua_setfield(L, -2, "canvasSize");
    script::push_value_to_lua<gfx::Rect>(L, roi.bounds());
    lua_setfield(L, -2, "bounds");
    script::setfield_integer(L, "frames", roi.frames());
    script::setfield_integer(L, "fromFrame", roi.fromFrame());
    script::setfield_integer(L, "toFrame", roi.toFrame());
    lua_pushboolean(L, fop->ignoreEmpty());
    lua_setfield(L, -2, "ignoreEmptyFrames");

    const auto call = lua_pcall(L, 4, 1, 0);
    closeFileHandle(L, fileHandle);

    if (call != LUA_OK) {
      if (const char* s = lua_tostring(L, -1))
        App::instance()->scriptEngine()->consolePrint(s);
      Console().printf("Could not save format '%s' to destination '%s'",
                       m_definition.name.c_str(),
                       fop->filename().c_str());
      return false;
    }

    if (!lua_toboolean(L, -1))
      return false;

    fop->document()->markAsSaved();
    return true;
  }
  #endif

private:
  void closeFileHandle(lua_State* L, int fileHandle)
  {
    lua_rawgeti(L, LUA_REGISTRYINDEX, fileHandle);
    auto* p = static_cast<luaL_Stream*>(luaL_testudata(L, -1, LUA_FILEHANDLE));
    if (p != nullptr && p->closef != nullptr) {
      const auto cf = p->closef;
      p->closef = nullptr;
      lua_pushcfunction(L, cf);
      lua_pushvalue(L, -2);
      lua_call(L, 1, 0);
    }
    luaL_unref(L, LUA_REGISTRYINDEX, fileHandle);
    lua_pop(L, 1);
  }

  Extension::CustomFormatDefinition m_definition;
  dio::FileFormat m_dioFormat;
  int m_flags;
  std::vector<Doc*> m_originalDocs;
  Doc* m_originalActiveDoc = nullptr;

  DISABLE_COPYING(ExtensionCustomFormat);
};

void Extension::addFileFormat(const CustomFormatDefinition& definition)
{
  auto* format = new ExtensionCustomFormat(definition);
  FileFormatsManager::instance()->registerFormat(format);

  m_fileFormats.push_back(format);
}
#endif

bool Extension::canBeDisabled() const
{
  return (m_isEnabled &&
          //! isCurrentTheme() &&
          !isDefaultTheme()); // Default theme cannot be disabled or uninstalled
}

bool Extension::canBeUninstalled() const
{
  return (!m_isBuiltinExtension &&
          // We can uninstall the current theme (e.g. to upgrade it)
          //! isCurrentTheme() &&
          !isDefaultTheme());
}

void Extension::enable(const bool state)
{
  if (m_isEnabled != state) {
    set_config_bool("extensions", m_name.c_str(), state);
    flush_config_file();

    m_isEnabled = state;
  }

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

void Extension::uninstall(const DeletePluginPref delPref)
{
  if (!m_isInstalled)
    return;

  ASSERT(canBeUninstalled());
  if (!canBeUninstalled())
    return;

  TRACE("EXT: Uninstall extension '%s' from '%s'...\n", m_name.c_str(), m_path.c_str());

  // Execute exit actions of scripts
  executeExitActions();

  // Remove all files inside the extension path
  uninstallFiles(m_path, delPref);

  m_isEnabled = false;
  m_isInstalled = false;
}

void Extension::uninstallFiles(const std::string& path, const DeletePluginPref delPref) const
{
  const std::string infoFn = base::join_path(path, kInfoJson);
  if (!base::is_file(infoFn))
    throw base::Exception("Cannot remove extension, '%s' file doesn't exist", infoFn.c_str());

  json11::Json json;
  read_json_file(infoFn, json);

  base::paths installedDirs;

  for (const auto& value : json["installedFiles"].array_items()) {
    const std::string& fn = base::join_path(path, value.string_value());
    if (base::is_file(fn)) {
      TRACE("EXT: Deleting file '%s'\n", fn.c_str());
      base::delete_file(fn);
    }
    else if (base::is_directory(fn)) {
      installedDirs.push_back(fn);
    }
  }

  // Delete __pref.lua file (only if specified, e.g. if the user is
  // updating the extension, the preferences should be kept).
  bool hasPrefFile = false;
  {
    const std::string& fn = base::join_path(path, kPrefLua);
    if (base::is_file(fn)) {
      if (delPref == DeletePluginPref::Yes)
        base::delete_file(fn);
      else
        hasPrefFile = true;
    }
  }

  std::sort(installedDirs.begin(),
            installedDirs.end(),
            [](const std::string& a, const std::string& b) { return b.size() < a.size(); });

  for (const auto& dir : installedDirs) {
    TRACE("EXT: Deleting directory '%s'\n", dir.c_str());
    try {
      base::remove_directory(dir);
    }
    catch (const std::exception& ex) {
      LOG(ERROR,
          "RECO: Extension subdirectory cannot be removed, it's not empty.\n"
          "      Error: %s\n",
          ex.what());
    }
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
  if (!hasPrefFile) {
    try {
      base::remove_directory(path);
    }
    catch (const std::exception& ex) {
      LOG(ERROR,
          "RECO: Extension directory cannot be removed, it's not empty.\n"
          "      Error: %s\n",
          ex.what());
    }
  }
}

bool Extension::isCurrentTheme() const
{
  const auto it = m_themes.find(Preferences::instance().theme.selected());
  return (it != m_themes.end());
}

bool Extension::isDefaultTheme() const
{
  return (name() == kAsepriteDefaultThemeExtensionName);
}

void Extension::updateCategory(const Category newCategory)
{
  if (m_category == Category::None || m_category == Category::Keys) {
    m_category = newCategory;
  }
  else if (m_category != newCategory)
    m_category = Category::Multiple;
}

#ifdef ENABLE_SCRIPTING

static bool is_simple_id(const char* k)
{
  if (!*k)
    return false;
  if (!std::isalpha(*k) && *k != '_')
    return false;
  for (const char* p = k + 1; *p; ++p) {
    if (!std::isalnum(*p) && *p != '_')
      return false;
  }
  return true;
}

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
        // If this is a simple identifier we can avoid using ["..."] to enclose
        // the key name.
        if (is_simple_id(k)) {
          result += k;
        }
        // In other cases, we enclose the key name in brackets and
        // double quotes ["..."]  to avoid syntax errors when the name
        // has unicode characters.
        else {
          result.push_back('[');
          result.push_back('"');
          result += k;
          result.push_back('"');
          result.push_back(']');
        }
        result.push_back('=');
      }
    }

    // Save value
    switch (lua_type(L, -1)) {
      case LUA_TNIL:
      default:       result += "nil"; break;
      case LUA_TBOOLEAN:
        if (lua_toboolean(L, -1))
          result += "true";
        else
          result += "false";
        break;
      case LUA_TNUMBER: result += lua_tostring(L, -1); break;
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
              default: result.push_back(*p); break;
            }
          }
        }
        result.push_back('\"');
        break;
      case LUA_TTABLE: serialize_table(L, -1, result); break;
    }
    lua_pop(L, 1);
  }

  result.push_back('}');
}

Extension::ScriptItem::ScriptItem(const std::string& fn) : fn(fn), exitFunctionRef(LUA_REFNIL)
{
}

void Extension::initScripts()
{
  script::Engine* engine = App::instance()->scriptEngine();
  lua_State* L = engine->luaState();

  // Put a new "plugin" object for init()/exit() functions
  script::push_plugin(L, this);
  m_plugin.pluginRef = luaL_ref(L, LUA_REGISTRYINDEX);

  // Set the _PLUGIN global so require() can find .lua files from the
  // plugin path.
  const script::SetPluginForRequire setPlugin(L, m_plugin.pluginRef);

  // Read plugin.preferences value
  {
    const std::string& fn = base::join_path(m_path, kPrefLua);
    if (base::is_file(fn)) {
      lua_rawgeti(L, LUA_REGISTRYINDEX, m_plugin.pluginRef);
      if (luaL_loadfile(L, fn.c_str()) == LUA_OK) {
        if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
          lua_setfield(L, -2, "preferences");
        }
        else if (const char* s = lua_tostring(L, -1)) {
          Console().printf("%s\n", s);
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

    // Call the init() function of this script with a Plugin object as first parameter
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
  // Delete all file formats.
  for (auto* format : m_fileFormats) {
    FileFormatsManager::instance()->unregisterFormat(format);
    delete format;
  }
  m_fileFormats.clear();

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
      lua_pop(L, 2); // Remove the value and the key

    if (hasPreferences) {
      std::string result = "return ";
      serialize_table(L, -1, result);
      base::write_file_content(base::join_path(m_path, kPrefLua),
                               (const uint8_t*)result.c_str(),
                               result.size());
    }
    lua_pop(L, 2); // Pop preferences table and plugin

    luaL_unref(L, LUA_REGISTRYINDEX, m_plugin.pluginRef);
    m_plugin.pluginRef = LUA_REFNIL;
  }

  // Remove plugin items automatically from back to front (in the
  // reverse order that they were added).
  for (auto it = m_plugin.items.rbegin(), end = m_plugin.items.rend(); it != end; ++it) {
    auto& item = *it;

    switch (item.type) {
      case PluginItem::Command: {
        auto* cmds = Commands::instance();
        auto* cmd = cmds->byId(item.id.c_str());
        ASSERT(cmd);
        if (cmd) {
          MenuItemRemoveCommand(cmd);

          cmds->remove(cmd);

          // This will call ~PluginCommand() and unref the command
          // onclick callback.
          delete cmd;
        }
        break;
      }

      case PluginItem::MenuSeparator:
        ASSERT(item.widget);
        ASSERT(item.widget->parent());
        if (item.widget && item.widget->parent()) {
          MenuItemRemoveWidget(item.widget);
          ASSERT(!item.widget->parent());
          item.widget = nullptr;
        }
        break;

      case PluginItem::MenuGroup: MenuGroupRemove(item.id); break;
    }
  }

  m_plugin.items.clear();
}

void Extension::addScript(const std::string& fn)
{
  m_plugin.scripts.emplace_back(fn);
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
    rf2.includeUserDir("extensions");
    m_userExtensionsPath = rf2.getFirstOrCreateDefault();
    m_userExtensionsPath = base::normalize_path(m_userExtensionsPath);
    if (!m_userExtensionsPath.empty() && m_userExtensionsPath.back() == '.') {
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
    const auto& extensionsDir = rf.filename();

    if (!base::is_directory(extensionsDir))
      continue;

    for (const auto& fn : base::list_files(extensionsDir, base::ItemType::Directories)) {
      const auto dir = base::join_path(extensionsDir, fn);
      const bool isBuiltinExtension = (m_userExtensionsPath != base::get_file_path(dir));

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
        LOG("EXT: Error loading JSON file: %s\n", ex.what());
      }
    }
  }
}

Extensions::~Extensions()
{
  for (const auto* ext : m_extensions)
    delete ext;
}

void Extensions::executeInitActions()
{
  for (const auto& ext : m_extensions)
    ext->executeInitActions();

  ScriptsChange(nullptr);
}

void Extensions::executeExitActions()
{
  for (const auto& ext : m_extensions)
    ext->executeExitActions();

  ScriptsChange(nullptr);
}

std::string Extensions::languagePath(const std::string& langId) const
{
  for (const auto& ext : m_extensions) {
    if (!ext->isEnabled()) // Ignore disabled extensions
      continue;

    auto it = ext->languages().find(langId);
    if (it != ext->languages().end())
      return it->second.path;
  }
  return std::string();
}

std::string Extensions::themePath(const std::string& themeId) const
{
  for (const auto* ext : m_extensions) {
    if (!ext->isEnabled()) // Ignore disabled extensions
      continue;

    auto it = ext->themes().find(themeId);
    if (it != ext->themes().end())
      return it->second.path;
  }
  return std::string();
}

std::string Extensions::palettePath(const std::string& palId) const
{
  for (const auto* ext : m_extensions) {
    if (!ext->isEnabled()) // Ignore disabled extensions
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
  for (const auto* ext : m_extensions) {
    if (!ext->isEnabled()) // Ignore disabled themes
      continue;

    for (const auto& [id, path] : ext->palettes())
      palettes[id] = path;
  }
  return palettes;
}

const render::DitheringMatrix* Extensions::ditheringMatrix(const std::string& matrixId) const
{
  for (const auto* ext : m_extensions) {
    if (!ext->isEnabled()) // Ignore disabled themes
      continue;

    auto it = ext->m_ditheringMatrices.find(matrixId);
    if (it != ext->m_ditheringMatrices.end())
      return &it->second.matrix();
  }
  return nullptr;
}

std::vector<Extension::DitheringMatrixInfo*> Extensions::ditheringMatrices()
{
  std::vector<Extension::DitheringMatrixInfo*> result;
  for (auto* ext : m_extensions) {
    if (!ext->isEnabled()) // Ignore disabled themes
      continue;

    for (auto& it : ext->m_ditheringMatrices)
      result.push_back(&it.second);
  }
  return result;
}

void Extensions::enableExtension(Extension* extension, const bool state)
{
  extension->enable(state);
  generateExtensionSignals(extension);
}

void Extensions::uninstallExtension(Extension* extension, const DeletePluginPref delPref)
{
  extension->uninstall(delPref);
  generateExtensionSignals(extension);

  auto it = std::find(m_extensions.begin(), m_extensions.end(), extension);
  ASSERT(it != m_extensions.end());
  if (it != m_extensions.end())
    m_extensions.erase(it);

  delete extension;
}

ExtensionInfo Extensions::getCompressedExtensionInfo(const std::string& zipFn) const
{
  ExtensionInfo info;
  info.dstPath = base::join_path(m_userExtensionsPath, base::get_file_title(zipFn));

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
    if (!info.commonPath.empty() && entryFn.size() > info.commonPath.size()) {
      info.commonPath.push_back(entryFn[info.commonPath.size()]);
    }

    std::stringstream out;
    in.copyDataTo(out);

    std::string err;
    auto json = json11::Json::parse(out.str(), err);
    if (err.empty()) {
      if (json["contributes"].is_object()) {
        auto themes = json["contributes"]["themes"];
        if (json["name"].string_value() == Extension::kAsepriteDefaultThemeExtensionName)
          info.defaultTheme = true;
        else {
          if (themes.is_array()) {
            for (int i = 0; i < themes.array_items().size(); i++) {
              if (themes[i]["id"].string_value() == Extension::kAsepriteDefaultThemeId) {
                info.defaultTheme = true;
                break;
              }
            }
          }
        }
      }
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
      // some users are distributing extensions with the __info.json
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

      LOG("EXT: Uncompressing file <%s> to <%s>\n", fn.c_str(), fullFn.c_str());

      out.writeEntry(in, entry);
    }
  }

  // Save the list of installed files in "__info.json" file
  {
    json11::Json::object obj;
    obj["installedFiles"] = json11::Json(installedFiles);
    const json11::Json json(obj);

    const std::string fullFn = base::join_path(info.dstPath, kInfoJson);
    LOG("EXT: Saving list of installed files in <%s>\n", fullFn.c_str());
    write_json_file(fullFn, json);
  }

  // Load the extension
  Extension* extension =
    loadExtension(info.dstPath, base::join_path(info.dstPath, kPackageJson), false);
  if (!extension)
    throw base::Exception("Error adding the new extension");

  // Generate signals
  generateExtensionSignals(extension);

  return extension;
}

Extension* Extensions::loadExtension(const std::string& path,
                                     const std::string& fullPackageFilename,
                                     const bool isBuiltinExtension)
{
  json11::Json json;
  read_json_file(fullPackageFilename, json);
  const auto& name = json["name"].string_value();
  const auto& version = json["version"].string_value();
  const auto& displayName = json["displayName"].string_value();

  LOG("EXT: Extension '%s' loaded\n", name.c_str());

#if ENABLE_SENTRY
  if (!isBuiltinExtension) {
    std::map<std::string, std::string> data = {
      { "name",    name    },
      { "version", version }
    };
    if (json["author"].is_object()) {
      data["url"] = json["author"]["url"].string_value();
    }
    Sentry::addBreadcrumb("Load extension", data);
  }
#endif

  auto extension = std::make_unique<Extension>(path,
                                               name,
                                               version,
                                               displayName,
                                               // Extensions are enabled by default
                                               get_config_bool("extensions", name.c_str(), true),
                                               isBuiltinExtension);

  const auto& contributes = json["contributes"];
  if (contributes.is_object()) {
    // Keys
    auto keys = contributes["keys"];
    if (keys.is_array()) {
      for (const auto& key : keys.array_items()) {
        const std::string& keyId = key["id"].string_value();
        std::string keyPath = key["path"].string_value();

        // The path must be always relative to the extension
        keyPath = base::join_path(path, keyPath);

        LOG("EXT: New keyboard shortcuts '%s' in '%s'\n", keyId.c_str(), keyPath.c_str());

        extension->addKeys(keyId, keyPath);
      }
    }

    // Languages
    const auto& languages = contributes["languages"];
    if (languages.is_array()) {
      for (const auto& lang : languages.array_items()) {
        const std::string& langId = lang["id"].string_value();
        std::string langPath = lang["path"].string_value();
        const std::string& langDisplayName = lang["displayName"].string_value();

        // The path must be always relative to the extension
        langPath = base::join_path(path, langPath);

        LOG("EXT: New language id=%s path=%s displayName=%s\n",
            langId.c_str(),
            langPath.c_str(),
            langDisplayName.c_str());

        extension->addLanguage(langId, langPath, langDisplayName);
      }
    }

    // Themes
    const auto& themes = contributes["themes"];
    if (themes.is_array()) {
      for (const auto& theme : themes.array_items()) {
        const std::string& themeId = theme["id"].string_value();
        std::string themePath = theme["path"].string_value();
        const std::string& themeVariant = theme["variant"].string_value();

        // The path must be always relative to the extension
        themePath = base::join_path(path, themePath);

        LOG("EXT: New theme id=%s path=%s variant=%s\n",
            themeId.c_str(),
            themePath.c_str(),
            themeVariant.c_str());

        extension->addTheme(themeId, themePath, themeVariant);
      }
    }

    // Palettes
    const auto& palettes = contributes["palettes"];
    if (palettes.is_array()) {
      for (const auto& palette : palettes.array_items()) {
        const std::string& palId = palette["id"].string_value();
        std::string palPath = palette["path"].string_value();

        // The path must be always relative to the extension
        palPath = base::join_path(path, palPath);

        LOG("EXT: New palette id=%s path=%s\n", palId.c_str(), palPath.c_str());

        extension->addPalette(palId, palPath);
      }
    }

    // Dithering matrices
    const auto& ditheringMatrices = contributes["ditheringMatrices"];
    if (ditheringMatrices.is_array()) {
      for (const auto& ditheringMatrix : ditheringMatrices.array_items()) {
        const std::string& matId = ditheringMatrix["id"].string_value();
        std::string matPath = ditheringMatrix["path"].string_value();
        std::string matName = ditheringMatrix["name"].string_value();
        if (matName.empty())
          matName = matId;

        // The path must be always relative to the extension
        matPath = base::join_path(path, matPath);

        LOG("EXT: New dithering matrix id=%s path=%s\n", matId.c_str(), matPath.c_str());

        extension->addDitheringMatrix(matId, matPath, matName);
      }
    }

#ifdef ENABLE_SCRIPTING
    // Scripts
    const auto& scripts = contributes["scripts"];
    if (scripts.is_array()) {
      for (const auto& script : scripts.array_items()) {
        std::string scriptPath = script["path"].string_value();
        if (scriptPath.empty())
          continue;

        // The path must be always relative to the extension
        scriptPath = base::join_path(path, scriptPath);

        LOG("EXT: New script path=%s\n", scriptPath.c_str());

        extension->addScript(scriptPath);
      }
    }
    // Simple version of packages.json with {... "scripts": "file.lua" ...}
    else if (scripts.is_string() && !scripts.string_value().empty()) {
      std::string scriptPath = scripts.string_value();

      // The path must be always relative to the extension
      scriptPath = base::join_path(path, scriptPath);

      LOG("EXT: New script path=%s\n", scriptPath.c_str());

      extension->addScript(scriptPath);
    }

    if (AppMenus::instance()) {
      extension->MenuItemRemoveWidget.connect(&AppMenus::removeMenuItemFromGroup,
                                              AppMenus::instance());
      extension->MenuItemRemoveCommand.connect(&AppMenus::removeMenuItemFromGroup,
                                               AppMenus::instance());
      extension->MenuGroupRemove.connect(&AppMenus::removeMenuGroup, AppMenus::instance());
    }
#endif // ENABLE_SCRIPTING
  }

  if (extension)
    m_extensions.push_back(extension.get());
  return extension.release();
}

void Extensions::generateExtensionSignals(Extension* extension)
{
  if (extension->hasKeys())
    KeysChange(extension);
  if (extension->hasLanguages())
    LanguagesChange(extension);
  if (extension->hasThemes())
    ThemesChange(extension);
  if (extension->hasPalettes())
    PalettesChange(extension);
  if (extension->hasDitheringMatrices())
    DitheringMatricesChange(extension);
#ifdef ENABLE_SCRIPTING
  if (extension->hasScripts())
    ScriptsChange(extension);
#endif
}

} // namespace app
