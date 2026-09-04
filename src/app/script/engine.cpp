// Aseprite
// Copyright (C) 2018-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/script/engine.h"

#include "app/app.h"
#include "app/doc_exporter.h"
#include "app/doc_range.h"
#include "app/file/file_format.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/script/blend_mode.h"
#include "app/script/debugger.h"
#include "app/script/events.h"
#include "app/script/luacpp.h"
#include "app/script/permission_dialog.h"
#include "app/script/permission_storage.h"
#include "app/sprite_sheet_type.h"
#include "app/tilemap_mode.h"
#include "app/tileset_mode.h"
#include "app/tools/ink_type.h"
#include "app/ui_context.h"
#include "base/chrono.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "base/mem_utils.h"
#include "base/replace_string.h"
#include "doc/algorithm/flip_type.h"
#include "doc/anidir.h"
#include "doc/color_mode.h"
#include "filters/target.h"
#include "fmt/format.h"
#include "ui/base.h"
#include "ui/cursor_type.h"
#include "ui/mouse_button.h"
#include "ui/system.h"
#include "ver/info.h"

#include <fstream>
#include <sstream>
#include <stack>
#include <string>

// We use our own fopen() that supports Unicode filename on Windows
// extern "C"
FILE* lua_user_fopen(const char* fname, const char* mode)
{
  return base::open_file_raw(fname, mode);
}

FILE* lua_user_freopen(const char* fname, const char* mode, FILE* stream)
{
  return base::reopen_file_raw(fname, mode, stream);
}

namespace app::script {

Engine* get_engine(lua_State* L)
{
  auto* ptr = lua_getextraspace(L);
  return *static_cast<Engine**>(ptr);
}

void engine_print(lua_State* L, const std::string& message)
{
  get_engine(L)->ConsolePrint(message);
}

namespace {
int dofilecont(lua_State* L, int d1, lua_KContext d2)
{
  (void)d1;
  (void)d2;
  return lua_gettop(L) - 1;
}

// Helper function for file operations
int file_result(lua_State* L, bool result, int errorNo = 0, const std::string& fileName = "")
{
  if (result) {
    lua_pushboolean(L, 1);
    return 1;
  }

  luaL_pushfail(L);
  if (fileName.empty())
    lua_pushstring(L, strerror(errorNo));
  else
    lua_pushfstring(L, "%s: %s", fileName.c_str(), strerror(errorNo));
  lua_pushinteger(L, errorNo);
  return 3;
}

// Wraps member functions to be registered directly to Lua.
using member_function_t = int (Engine::*)();
template<member_function_t function>
int wrap(lua_State* L)
{
  Engine* ptr = *static_cast<Engine**>(lua_getextraspace(L));
  return (ptr->*function)();
}

using member_hook_t = void (Engine::*)(lua_Debug*) const;
template<member_hook_t function>
void wrap_hook(lua_State* L, lua_Debug* ar)
{
  const Engine* ptr = *static_cast<Engine**>(lua_getextraspace(L));
  (ptr->*function)(ar);
}

// Functions like the Lua allocator except filling the Engine memory tracker and
// stopping allocations when its limit is reached (if any).
void* tracking_allocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
  ASSERT(ud);
  auto* tracker = static_cast<Engine::MemoryTracker*>(ud);
  if (nsize == 0) {
    if (ptr != nullptr) {
      base_free(ptr);
      tracker->usage -= osize;
    }
    return nullptr;
  }

  // Memory usage limiter
  if (tracker->limit > 0 && tracker->usage + nsize >= tracker->limit) {
    LOG(ERROR,
        "Script memory limit hit: Using %s, attempted to allocate %s.\n",
        base::get_pretty_memory_size(tracker->usage).c_str(),
        base::get_pretty_memory_size(nsize).c_str());
    return nullptr;
  }
  tracker->usage += nsize;
  if (ptr != nullptr)
    tracker->usage -= osize;
  return base_realloc(ptr, nsize);
}

int unsupported_error(lua_State* L)
{
  lua_Debug ar;
  lua_getstack(L, 0, &ar);
  lua_getinfo(L, "n", &ar);
  if (ar.name)
    return luaL_error(L, "unsupported function '%s'", ar.name);
  return luaL_error(L, "unsupported function");
}

struct PackagePath {
  std::string previousPath;
  lua_State* L;
  std::stack<std::string>& stack;
  PackagePath(lua_State* L, const std::string& filename, std::stack<std::string>& stack)
    : L(L)
    , stack(stack)
  {
    lua_getglobal(L, "package");
    lua_getfield(L, -1, "path");
    previousPath = lua_tostring(L, -1);

    lua_pop(L, 1);
    lua_pushstring(L,
                   fmt::format("{};{}/?.lua",
                               previousPath,
                               base::get_file_path(base::get_absolute_path(filename)))
                     .c_str());
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);

    stack.push(filename);
  }

  ~PackagePath()
  {
    lua_getglobal(L, "package");
    lua_pushstring(L, previousPath.c_str());
    lua_setfield(L, -2, "path");
    lua_pop(L, 1);

    stack.pop();
  }
};

// Stores the original C functions that we end up replacing
struct {
  lua_CFunction dofile = nullptr;
  lua_CFunction loadfile = nullptr;
  lua_CFunction os_execute = nullptr;

  lua_CFunction io_open = nullptr;
  lua_CFunction io_popen = nullptr;
  lua_CFunction io_lines = nullptr;
  lua_CFunction io_input = nullptr;
  lua_CFunction io_output = nullptr;
  lua_CFunction io_tmpfile = nullptr;
  lua_CFunction package_loadlib = nullptr;

  lua_CFunction debug_getuservalue = nullptr;
  lua_CFunction debug_gethook = nullptr;
  lua_CFunction debug_getinfo = nullptr;
  lua_CFunction debug_getlocal = nullptr;
  lua_CFunction debug_getregistry = nullptr;
  lua_CFunction debug_getmetatable = nullptr;
  lua_CFunction debug_getupvalue = nullptr;
  lua_CFunction debug_upvaluejoin = nullptr;
  lua_CFunction debug_upvalueid = nullptr;
  lua_CFunction debug_setuservalue = nullptr;
  lua_CFunction debug_sethook = nullptr;
  lua_CFunction debug_setlocal = nullptr;
  lua_CFunction debug_setmetatable = nullptr;
  lua_CFunction debug_setupvalue = nullptr;
  lua_CFunction debug_traceback = nullptr;
  lua_CFunction debug_setcstacklimit = nullptr;
} g_original;

// Replaces the lua function with our own member function implementation
#define ENGINE_REPLACE_FUNC(PKG, FUNC)                                                             \
  if (!g_original.PKG##_##FUNC) {                                                                  \
    lua_getfield(L, -1, #FUNC);                                                                    \
    g_original.PKG##_##FUNC = lua_tocfunction(L, -1);                                              \
    lua_pop(L, 1);                                                                                 \
  }                                                                                                \
  lua_pushcfunction(L, &wrap<&Engine::lua_##PKG##_##FUNC>);                                        \
  lua_setfield(L, -2, #FUNC)

// Same as the previous one, but with a generic implementation that only gates behind one permission
#define ENGINE_GATE_FUNC(PKG, FUNC, PERMISSION)                                                    \
  if (!g_original.PKG##_##FUNC) {                                                                  \
    lua_getfield(L, -1, #FUNC);                                                                    \
    g_original.PKG##_##FUNC = lua_tocfunction(L, -1);                                              \
    lua_pop(L, 1);                                                                                 \
  }                                                                                                \
  lua_pushcfunction(L, [](lua_State* L) -> int {                                                   \
    get_engine(L)->accessGate(PERMISSION);                                                         \
    return g_original.PKG##_##FUNC(L);                                                             \
  });                                                                                              \
  lua_setfield(L, -2, #FUNC)

// Equivalent to what's declared in luaL_openlibs except without coroutines.
constexpr luaL_Reg lua_libraries[] = {
  { LUA_GNAME,       luaopen_base    },
  { LUA_LOADLIBNAME, luaopen_package },
  { LUA_TABLIBNAME,  luaopen_table   },
  { LUA_IOLIBNAME,   luaopen_io      },
  { LUA_OSLIBNAME,   luaopen_os      },
  { LUA_STRLIBNAME,  luaopen_string  },
  { LUA_MATHLIBNAME, luaopen_math    },
  { LUA_UTF8LIBNAME, luaopen_utf8    },
  { LUA_DBLIBNAME,   luaopen_debug   }
};
} // namespace

lua_CFunction engine_io_open()
{
  return g_original.io_open;
}

Engine::Engine() : L(nullptr), m_printEvalResult(false), m_returnCode(0), m_objectTracker(0)
{
  L = lua_newstate(tracking_allocator, &m_tracker);

#if _DEBUG
  const int top = lua_gettop(L);
#endif

  // Register a pointer to the current engine object
  *static_cast<Engine**>(lua_getextraspace(L)) = this;

  // Load Lua libraries
  for (const auto& [name, func] : lua_libraries) {
    luaL_requiref(L, name, func, 1);
    lua_pop(L, 1); // remove lib
  }

  // Security
  lua_getglobal(L, "os");
  lua_pushcfunction(L, unsupported_error);
  lua_setfield(L, -2, "exit");
  lua_pushcfunction(L, &wrap<&Engine::lua_os_clock>);
  lua_setfield(L, -2, "clock");
  lua_pushcfunction(L, &wrap<&Engine::lua_os_tmpname>);
  lua_setfield(L, -2, "tmpname");
  lua_pushcfunction(L, &wrap<&Engine::lua_os_remove>);
  lua_setfield(L, -2, "remove");
  lua_pushcfunction(L, &wrap<&Engine::lua_os_rename>);
  lua_setfield(L, -2, "rename");
  ENGINE_REPLACE_FUNC(os, execute);
  lua_pop(L, 1);

  lua_getglobal(L, "io");
  ENGINE_REPLACE_FUNC(io, open);
  ENGINE_REPLACE_FUNC(io, popen);
  ENGINE_REPLACE_FUNC(io, lines);
  ENGINE_REPLACE_FUNC(io, input);
  ENGINE_REPLACE_FUNC(io, output);
  ENGINE_GATE_FUNC(io, tmpfile, Permission::TemporaryFile);
  lua_pop(L, 1);

  lua_getglobal(L, "package");
  ENGINE_REPLACE_FUNC(package, loadlib);
  lua_pop(L, 1);

  lua_getglobal(L, "debug");
  lua_pushcfunction(L, unsupported_error);
  lua_setfield(L, -2, "debug");
  ENGINE_GATE_FUNC(debug, getuservalue, Permission::Debug);
  ENGINE_GATE_FUNC(debug, gethook, Permission::Debug);
  ENGINE_GATE_FUNC(debug, getinfo, Permission::Debug);
  ENGINE_GATE_FUNC(debug, getlocal, Permission::Debug);
  ENGINE_GATE_FUNC(debug, getregistry, Permission::Debug);
  ENGINE_GATE_FUNC(debug, getmetatable, Permission::Debug);
  ENGINE_GATE_FUNC(debug, getupvalue, Permission::Debug);
  ENGINE_GATE_FUNC(debug, upvaluejoin, Permission::Debug);
  ENGINE_GATE_FUNC(debug, upvalueid, Permission::Debug);
  ENGINE_GATE_FUNC(debug, setuservalue, Permission::Debug);
  ENGINE_GATE_FUNC(debug, sethook, Permission::Debug);
  ENGINE_GATE_FUNC(debug, setlocal, Permission::Debug);
  ENGINE_GATE_FUNC(debug, setmetatable, Permission::Debug);
  ENGINE_GATE_FUNC(debug, setupvalue, Permission::Debug);
  ENGINE_GATE_FUNC(debug, traceback, Permission::Debug);
  ENGINE_GATE_FUNC(debug, setcstacklimit, Permission::Debug);
  lua_pop(L, 1);

  // Replacing global functions
  lua_register(L, "print", &wrap<&Engine::lua_print>);
  if (!g_original.dofile) {
    lua_getglobal(L, "dofile");
    g_original.dofile = lua_tocfunction(L, -1);
    lua_pop(L, 1);
  }
  lua_register(L, "dofile", &wrap<&Engine::lua_dofile>);
  if (!g_original.loadfile) {
    lua_getglobal(L, "loadfile");
    g_original.loadfile = lua_tocfunction(L, -1);
    lua_pop(L, 1);
  }
  lua_register(L, "loadfile", &wrap<&Engine::lua_loadfile>);

  // Register constants
  lua_createtable(L, 0, 5);
  setfield_integer(L, "RGB", doc::ColorMode::RGB);
  setfield_integer(L, "GRAY", doc::ColorMode::GRAYSCALE);
  setfield_integer(L, "GRAYSCALE", doc::ColorMode::GRAYSCALE);
  setfield_integer(L, "INDEXED", doc::ColorMode::INDEXED);
  setfield_integer(L, "TILEMAP", doc::ColorMode::TILEMAP);
  lua_setglobal(L, "ColorMode");

  lua_createtable(L, 0, 4);
  setfield_integer(L, "FORWARD", doc::AniDir::FORWARD);
  setfield_integer(L, "REVERSE", doc::AniDir::REVERSE);
  setfield_integer(L, "PING_PONG", doc::AniDir::PING_PONG);
  setfield_integer(L, "PING_PONG_REVERSE", doc::AniDir::PING_PONG_REVERSE);
  lua_setglobal(L, "AniDir");

  lua_createtable(L, 0, 37);
  setfield_integer(L, "CLEAR", app::script::BlendMode::CLEAR);
  setfield_integer(L, "SRC", app::script::BlendMode::SRC);
  setfield_integer(L, "DST", app::script::BlendMode::DST);
  setfield_integer(L, "SRC_OVER", app::script::BlendMode::SRC_OVER);
  setfield_integer(L, "DST_OVER", app::script::BlendMode::DST_OVER);
  setfield_integer(L, "SRC_IN", app::script::BlendMode::SRC_IN);
  setfield_integer(L, "DST_IN", app::script::BlendMode::DST_IN);
  setfield_integer(L, "SRC_OUT", app::script::BlendMode::SRC_OUT);
  setfield_integer(L, "DST_OUT", app::script::BlendMode::DST_OUT);
  setfield_integer(L, "SRC_ATOP", app::script::BlendMode::SRC_ATOP);
  setfield_integer(L, "DST_ATOP", app::script::BlendMode::DST_ATOP);
  setfield_integer(L, "XOR", app::script::BlendMode::XOR);
  setfield_integer(L, "PLUS", app::script::BlendMode::PLUS);
  setfield_integer(L, "MODULATE", app::script::BlendMode::MODULATE);
  setfield_integer(L, "MULTIPLY", app::script::BlendMode::MULTIPLY);
  setfield_integer(L, "SCREEN", app::script::BlendMode::SCREEN);
  setfield_integer(L, "OVERLAY", app::script::BlendMode::OVERLAY);
  setfield_integer(L, "DARKEN", app::script::BlendMode::DARKEN);
  setfield_integer(L, "LIGHTEN", app::script::BlendMode::LIGHTEN);
  setfield_integer(L, "COLOR_DODGE", app::script::BlendMode::COLOR_DODGE);
  setfield_integer(L, "COLOR_BURN", app::script::BlendMode::COLOR_BURN);
  setfield_integer(L, "HARD_LIGHT", app::script::BlendMode::HARD_LIGHT);
  setfield_integer(L, "SOFT_LIGHT", app::script::BlendMode::SOFT_LIGHT);
  setfield_integer(L, "DIFFERENCE", app::script::BlendMode::DIFFERENCE);
  setfield_integer(L, "EXCLUSION", app::script::BlendMode::EXCLUSION);
  setfield_integer(L, "HUE", app::script::BlendMode::HUE);
  setfield_integer(L, "SATURATION", app::script::BlendMode::SATURATION);
  setfield_integer(L, "COLOR", app::script::BlendMode::COLOR);
  setfield_integer(L, "LUMINOSITY", app::script::BlendMode::LUMINOSITY);
  setfield_integer(L, "ADDITION", app::script::BlendMode::ADDITION);
  setfield_integer(L, "SUBTRACT", app::script::BlendMode::SUBTRACT);
  setfield_integer(L, "DIVIDE", app::script::BlendMode::DIVIDE);
  // Backward compatibility
  setfield_integer(L, "NORMAL", app::script::BlendMode::SRC_OVER);
  setfield_integer(L, "HSL_HUE", app::script::BlendMode::HUE);
  setfield_integer(L, "HSL_SATURATION", app::script::BlendMode::SATURATION);
  setfield_integer(L, "HSL_COLOR", app::script::BlendMode::COLOR);
  setfield_integer(L, "HSL_LUMINOSITY", app::script::BlendMode::LUMINOSITY);
  lua_setglobal(L, "BlendMode");

  lua_createtable(L, 0, 4);
  setfield_integer(L, "EMPTY", DocRange::kNone);
  setfield_integer(L, "LAYERS", DocRange::kLayers);
  setfield_integer(L, "FRAMES", DocRange::kFrames);
  setfield_integer(L, "CELS", DocRange::kCels);
  lua_setglobal(L, "RangeType");

  lua_createtable(L, 0, 5);
  setfield_integer(L, "HORIZONTAL", SpriteSheetType::Horizontal);
  setfield_integer(L, "VERTICAL", SpriteSheetType::Vertical);
  setfield_integer(L, "ROWS", SpriteSheetType::Rows);
  setfield_integer(L, "COLUMNS", SpriteSheetType::Columns);
  setfield_integer(L, "PACKED", SpriteSheetType::Packed);
  lua_setglobal(L, "SpriteSheetType");

  lua_createtable(L, 0, 2);
  setfield_integer(L, "JSON_HASH", SpriteSheetDataFormat::JsonHash);
  setfield_integer(L, "JSON_ARRAY", SpriteSheetDataFormat::JsonArray);
  lua_setglobal(L, "SpriteSheetDataFormat");

  lua_createtable(L, 0, 4);
  setfield_integer(L, "CIRCLE", doc::kCircleBrushType);
  setfield_integer(L, "SQUARE", doc::kSquareBrushType);
  setfield_integer(L, "LINE", doc::kLineBrushType);
  setfield_integer(L, "IMAGE", doc::kImageBrushType);
  lua_setglobal(L, "BrushType");

  lua_createtable(L, 0, 3);
  setfield_integer(L, "ORIGIN", doc::BrushPattern::ALIGNED_TO_SRC);
  setfield_integer(L, "TARGET", doc::BrushPattern::ALIGNED_TO_DST);
  setfield_integer(L, "NONE", doc::BrushPattern::PAINT_BRUSH);
  lua_setglobal(L, "BrushPattern");

  lua_createtable(L, 0, 5);
  setfield_integer(L, "SIMPLE", app::tools::InkType::SIMPLE);
  setfield_integer(L, "ALPHA_COMPOSITING", app::tools::InkType::ALPHA_COMPOSITING);
  setfield_integer(L, "COPY_COLOR", app::tools::InkType::COPY_COLOR);
  setfield_integer(L, "LOCK_ALPHA", app::tools::InkType::LOCK_ALPHA);
  setfield_integer(L, "SHADING", app::tools::InkType::SHADING);
  lua_setglobal(L, "Ink");

  lua_createtable(L, 0, 9);
  setfield_integer(L, "RED", TARGET_RED_CHANNEL);
  setfield_integer(L, "GREEN", TARGET_GREEN_CHANNEL);
  setfield_integer(L, "BLUE", TARGET_BLUE_CHANNEL);
  setfield_integer(L, "ALPHA", TARGET_ALPHA_CHANNEL);
  setfield_integer(L, "GRAY", TARGET_GRAY_CHANNEL);
  setfield_integer(L, "INDEX", TARGET_INDEX_CHANNEL);
  setfield_integer(L, "RGB", TARGET_RED_CHANNEL | TARGET_GREEN_CHANNEL | TARGET_BLUE_CHANNEL);
  setfield_integer(
    L,
    "RGBA",
    TARGET_RED_CHANNEL | TARGET_GREEN_CHANNEL | TARGET_BLUE_CHANNEL | TARGET_ALPHA_CHANNEL);
  setfield_integer(L, "GRAYA", TARGET_GRAY_CHANNEL | TARGET_ALPHA_CHANNEL);
  lua_setglobal(L, "FilterChannels");

  lua_createtable(L, 0, 18);
  setfield_integer(L, "NONE", (int)ui::kNoCursor);
  setfield_integer(L, "ARROW", (int)ui::kArrowCursor);
  setfield_integer(L, "CROSSHAIR", (int)ui::kCrosshairCursor);
  setfield_integer(L, "POINTER", (int)ui::kHandCursor);
  setfield_integer(L, "NOT_ALLOWED", (int)ui::kForbiddenCursor);
  setfield_integer(L, "GRAB", (int)ui::kScrollCursor);
  setfield_integer(L, "GRABBING", (int)ui::kScrollCursor);
  setfield_integer(L, "MOVE", (int)ui::kMoveCursor);
  setfield_integer(L, "NS_RESIZE", (int)ui::kSizeNSCursor);
  setfield_integer(L, "WE_RESIZE", (int)ui::kSizeWECursor);
  setfield_integer(L, "N_RESIZE", (int)ui::kSizeNCursor);
  setfield_integer(L, "NE_RESIZE", (int)ui::kSizeNECursor);
  setfield_integer(L, "E_RESIZE", (int)ui::kSizeECursor);
  setfield_integer(L, "SE_RESIZE", (int)ui::kSizeSECursor);
  setfield_integer(L, "S_RESIZE", (int)ui::kSizeSCursor);
  setfield_integer(L, "SW_RESIZE", (int)ui::kSizeSWCursor);
  setfield_integer(L, "W_RESIZE", (int)ui::kSizeWCursor);
  setfield_integer(L, "NW_RESIZE", (int)ui::kSizeNWCursor);
  lua_setglobal(L, "MouseCursor");

  lua_createtable(L, 0, 6);
  setfield_integer(L, "NONE", (int)ui::kButtonNone);
  setfield_integer(L, "LEFT", (int)ui::kButtonLeft);
  setfield_integer(L, "RIGHT", (int)ui::kButtonRight);
  setfield_integer(L, "MIDDLE", (int)ui::kButtonMiddle);
  setfield_integer(L, "X1", (int)ui::kButtonX1);
  setfield_integer(L, "X2", (int)ui::kButtonX2);
  lua_setglobal(L, "MouseButton");

  lua_createtable(L, 0, 2);
  setfield_integer(L, "PIXELS", TilemapMode::Pixels);
  setfield_integer(L, "TILES", TilemapMode::Tiles);
  lua_setglobal(L, "TilemapMode");

  lua_createtable(L, 0, 3);
  setfield_integer(L, "MANUAL", TilesetMode::Manual);
  setfield_integer(L, "AUTO", TilesetMode::Auto);
  setfield_integer(L, "STACK", TilesetMode::Stack);
  lua_setglobal(L, "TilesetMode");

  lua_createtable(L, 0, 4);
  setfield_integer(L, "REPLACE", (int)gen::SelectionMode::REPLACE);
  setfield_integer(L, "ADD", (int)gen::SelectionMode::ADD);
  setfield_integer(L, "SUBTRACT", (int)gen::SelectionMode::SUBTRACT);
  setfield_integer(L, "INTERSECT", (int)gen::SelectionMode::INTERSECT);
  lua_setglobal(L, "SelectionMode");

  lua_createtable(L, 0, 3);
  setfield_integer(L, "HORIZONTAL", doc::algorithm::FlipType::FlipHorizontal);
  setfield_integer(L, "VERTICAL", doc::algorithm::FlipType::FlipVertical);
  setfield_integer(L, "DIAGONAL", doc::algorithm::FlipType::FlipDiagonal);
  lua_setglobal(L, "FlipType");

  lua_createtable(L, 0, 5);
  setfield_integer(L, "LEFT", ui::LEFT);
  setfield_integer(L, "CENTER", ui::CENTER);
  setfield_integer(L, "RIGHT", ui::RIGHT);
  setfield_integer(L, "TOP", ui::TOP);
  setfield_integer(L, "BOTTOM", ui::BOTTOM);
  lua_setglobal(L, "Align");

  lua_createtable(L, 0, 9);
  setfield_integer(L, "RGB", FILE_SUPPORT_RGB);
  setfield_integer(L, "RGBA", FILE_SUPPORT_RGBA);
  setfield_integer(L, "GRAY", FILE_SUPPORT_GRAY);
  setfield_integer(L, "GRAYA", FILE_SUPPORT_GRAYA);
  setfield_integer(L, "INDEXED", FILE_SUPPORT_INDEXED);
  setfield_integer(L, "LAYER", FILE_SUPPORT_LAYERS);
  setfield_integer(L, "FRAME", FILE_SUPPORT_FRAMES);
  setfield_integer(L, "PALETTE", FILE_SUPPORT_PALETTES | FILE_SUPPORT_BIG_PALETTES);
  setfield_integer(L, "PALETTE_ALPHA", FILE_SUPPORT_PALETTE_WITH_ALPHA);
  lua_setglobal(L, "FormatSupport");

  // Call all the registration functions
  for (const auto fn : engine_registration_functions) {
    fn(L);
  }

  // Mark stdin file handle as closed so the following statements
  // don't hang the program:
  // - io.lines()
  // - io.read('a')
  // - io.stdin:read('a')
  const auto* app = App::instance();
  ASSERT(app);
  if (app && app->isGui()) {
    lua_getglobal(L, "io");
    lua_getfield(L, -1, "stdin");
    auto* p = ((luaL_Stream*)luaL_checkudata(L, -1, LUA_FILEHANDLE));
    ASSERT(p);
    p->f = nullptr;
    p->closef = nullptr;
    lua_pop(L, 2);
  }

  // Check that we have a clean start (without anything in the stack)
  ASSERT(lua_gettop(L) == top);
  ASSERT(m_tracker.usage > 0);

  // Mark initial memory usage after initialization.
  m_tracker.initialUsage = m_tracker.usage;
}

Engine::~Engine()
{
  // All files created with tmpname are deleted with the engine, matches Lua's behavior.
  for (const auto& file : m_temporaryFiles) {
    if (base::is_file(file)) {
      try {
        base::delete_file(file);
      }
      catch (const std::exception& e) {
        LOG(WARNING, "Unable to delete temporary file '%s': '%s'\n", file.c_str(), e.what());
      }
    }
  }

  lua_close(L);
  L = nullptr;
}

int Engine::lua_dofile()
{
  const char* argFname = luaL_optstring(L, 1, NULL);
  if (!argFname) {
    const auto* app = App::instance();
    if (app && app->isGui() && !lua_isstring(L, 1)) {
      return luaL_error(L, "dofile() for stdin cannot be used when running in GUI mode");
    }
    return g_original.dofile(L);
  }

  std::string filename(argFname);
  if (!base::is_file(filename)) {
    const auto top = m_scriptStack.empty() ? m_baseScript : m_scriptStack.top();

    // Try to complete a relative filename
    const std::string altFilename = base::join_path(base::get_file_path(top), filename);
    if (base::is_file(altFilename))
      filename = altFilename;
  }

  lua_settop(L, 1);
  if (luaL_loadfile(L, filename.c_str()) != LUA_OK)
    return lua_error(L);
  {
    const PackagePath set(L, filename, m_scriptStack);
    lua_callk(L, 0, LUA_MULTRET, 0, dofilecont);
  }

  return dofilecont(L, 0, 0);
}

int Engine::lua_loadfile()
{
  // fname is not optional if we are running in GUI mode as it blocks
  // the program.
  const auto* app = App::instance();
  if (app && app->isGui() && !lua_isstring(L, 1)) {
    return luaL_error(L, "loadfile() for stdin cannot be used when running in GUI mode");
  }
  return g_original.loadfile(L);
}

int Engine::lua_print()
{
  std::string output;
  int n = lua_gettop(L); /* number of arguments */
  lua_getglobal(L, "tostring");
  for (int i = 1; i <= n; i++) {
    lua_pushvalue(L, -1); // function to be called
    lua_pushvalue(L, i);  // value to print
    lua_call(L, 1, 1);
    size_t l;
    const char* s = lua_tolstring(L, -1, &l); // get result
    if (s == nullptr)
      return luaL_error(L, "'tostring' must return a string to 'print'");
    if (i > 1)
      output.push_back('\t');
    output.insert(output.size(), s, l);
    lua_pop(L, 1); // pop result
  }

  if (!output.empty())
    ConsolePrint(output);

  return 0;
}

int Engine::lua_io_input()
{
  return absoluteFilenameAccess(g_original.io_input);
}

int Engine::lua_io_lines()
{
  return absoluteFilenameAccess(g_original.io_lines);
}

int Engine::lua_os_clock()
{
  lua_pushnumber(L, m_clock.elapsed());
  return 1;
}

int Engine::lua_os_tmpname()
{
  const auto path = base::join_path(base::get_temp_path(), get_app_name());
  if (!base::is_directory(path)) {
    base::make_all_directories(path);
  }

  std::string filename;
  if (const auto* arg = luaL_optstring(L, 1, nullptr)) {
    std::string filenameArg = arg;
    // Attempts to ensure we're not trying to create a file in a directory outside the temp one
    if (std::find_if(filenameArg.begin(), filenameArg.end(), &base::is_path_separator) !=
        filenameArg.end()) {
      return luaL_error(L, "invalid filename");
    }
    filename = base::join_path(path, filenameArg);
  }
  else {
    filename = base::join_path(path, fmt::format("temp_{}{}", std::rand(), base::current_tick()));
  }
  accessGate(Permission::TemporaryFile);

  if (base::is_file(filename))
    return luaL_error(L, "temporary file already exists");

  {
    // Ensure the file is created for Lua compatibility. It only does this in Unix systems for
    // security, but we can do it for all platforms for consistency.
    base::open_file(filename, "w");
  }
  ASSERT(base::is_file(filename));
  m_temporaryFiles.emplace(filename);
  lua_pushstring(L, filename.c_str());
  return 1;
}

int Engine::lua_os_execute()
{
  const std::string cmd = luaL_checkstring(L, 1);
  if (cmd.empty()) {
    // Match Lua behavior.
    lua_pushboolean(L, 1);
    return 1;
  }
  accessGate(Permission::Execute, cmd);
  return g_original.os_execute(L);
}

int Engine::lua_os_remove()
{
  const std::string& path = base::get_canonical_path(luaL_checkstring(L, 1));
  if (path.empty())
    return file_result(L, false, ENOENT, path);

  if (!requestAccess(Permission::IOWrite, path))
    return file_result(L, false, EACCES, path);

  if (base::is_directory(path)) {
    try {
      base::remove_directory(path);
      return file_result(L, true);
    }
    catch (const std::exception& e) {
      LOG(WARNING, "Script failed to delete directory '%s': %s\n", path.c_str(), e.what());
      return file_result(L, false, EIO, path);
    }
  }

  try {
    base::delete_file(path);
  }
  catch (const std::exception& e) {
    LOG(WARNING, "Script failed to delete file '%s': %s\n", path.c_str(), e.what());
    return file_result(L, false, EIO, path);
  }

  return file_result(L, true);
}

int Engine::lua_os_rename()
{
  const std::string& source = base::get_canonical_path(luaL_checkstring(L, 1));
  const std::string& dest = base::get_absolute_path(luaL_checkstring(L, 2));
  lua_pop(L, 2);

  if (source.empty())
    return file_result(L, false, ENOENT, source);

  if (dest.empty())
    return file_result(L, false, EINVAL, dest);

  if (!requestAccess(Permission::IOWrite, source))
    return file_result(L, false, EACCES, source);

  try {
    // If the destination file already exists, we should ask for permission to overwrite it.
    if (!base::get_canonical_path(dest).empty() && !requestAccess(Permission::IOWrite, dest)) {
      return file_result(L, false, EACCES, dest);
    }

    base::move_file(source, dest, true);
    return file_result(L, true);
  }
  catch (const std::exception& e) {
    LOG(WARNING,
        "Script failed to rename file '%s' to '%s': %s\n",
        source.c_str(),
        dest.c_str(),
        e.what());
    return file_result(L, false, EIO, source);
  }
}

int Engine::lua_package_loadlib()
{
  const std::string file = luaL_checkstring(L, 1);
  if (file.empty())
    return 0;
  accessGate(Permission::LoadLib, file);
  return g_original.package_loadlib(L);
}

int Engine::lua_io_open()
{
  const std::string path = base::normalize_path(base::get_absolute_path(luaL_checkstring(L, 1)));

  if (m_temporaryFiles.find(path) != m_temporaryFiles.end()) {
    accessGate(Permission::TemporaryFile);
  }
  else {
    auto permission = Permission::IORead; // Read is the default access
    if (lua_isstring(L, 2)) {
      const std::string_view mode = lua_tostring(L, 2);
      if (!mode.empty() && (mode[0] == 'w' || mode[0] == 'a' || mode.substr(0, 2) == "r+"))
        permission = Permission::IOWrite;
    }

    accessGate(permission, path);
  }

  return g_original.io_open(L);
}

int Engine::lua_io_output()
{
  return absoluteFilenameAccess(g_original.io_output, false);
}

int Engine::lua_io_popen()
{
  const char* cmd = luaL_checkstring(L, 1);
  accessGate(Permission::Execute, cmd);
  return g_original.io_popen(L);
}

void Engine::lua_hook(lua_Debug* ar) const
{
  ASSERT(m_debugger)
  if (m_debugger)
    m_debugger->onHook(L, ar);
}

int Engine::absoluteFilenameAccess(const lua_CFunction func, const bool readOnly)
{
  if (const auto* fn = lua_tostring(L, 1)) {
    const std::string absFilename = base::get_absolute_path(fn);

    const auto permission = readOnly ? Permission::IORead : Permission::IOWrite;
    accessGate(permission, absFilename);
  }

  return func(L);
}

bool Engine::requestAccess(const Permission permission, const std::string& match)
{
  std::string script;
  if (m_extensionName.empty())
    script = m_scriptStack.empty() ? m_baseScript : m_scriptStack.top();
  else
    script = kExtensionPrefix + m_extensionName;

  // Only give REPL stuff access to non-scary permissions
  if (script.empty())
    return !permission_is_scary(permission);

  auto* app = App::instance();
  auto* storage = PermissionStorage::instance();
  if (m_extensionName.empty() && !app->preferences().developer.disableIntegrityCheck() &&
      script != m_baseScript && !storage->passesIntegrityCheck(script)) {
    // We check integrity when we run the userScript initially but that doesn't include scripts
    // that live in dofile or require, and those permissions belong to them individually and need
    // to be integrity checked.
    LOG(INFO, "Permission integrity failed for script '%s', resetting.\n", script.c_str());
    storage->reset(script);
  }

  std::optional<bool> opt;
  try {
    opt = permission_supports_matching(permission) ? storage->readMatch(script, permission, match) :
                                                     storage->read(script, permission);
  }
  catch (const std::exception& e) {
    LOG(WARNING,
        "Error while reading permission %s(%s): '%s'\n",
        permission,
        match.c_str(),
        e.what());
    // Any parsing failure should reset our storage.
    storage->reset(script);
  }

  if (!app->context()->isUIAvailable()) {
    if (opt.has_value() && *opt)
      return true;
    return app->preferences().general.allowCliScriptsFullAccess();
  }

  if (opt.has_value())
    return *opt;

  PermissionDialog dialog(script, m_extensionName, permission, match);
  auto [allow, remember] = dialog.ask();
  switch (remember) {
    case PermissionDialog::Remember::Nothing:    break;
    case PermissionDialog::Remember::FullAccess: storage->writeFullAccess(script, true); break;
    case PermissionDialog::Remember::Permission: storage->write(script, permission, allow); break;
    case PermissionDialog::Remember::AllOfType:  {
      storage->writeForMatch(script, permission, "*", allow);
      break;
    }
    case PermissionDialog::Remember::Match: {
      // Escape any asterisks that come from user input so that the match is literal.
      std::string safeMatch = match;
      base::replace_string(safeMatch, "*", "[*]");

      storage->writeForMatch(script, permission, safeMatch, allow);
      break;
    }
    case PermissionDialog::Remember::Directory: {
      const std::string dirMatch = base::join_path(base::get_file_path(match), "*");
      storage->writeForMatch(script, permission, dirMatch, allow);
      break;
    }
  }

  return allow;
}

void Engine::accessGate(const Permission permission, const std::string& match)
{
  if (permission_supports_matching(permission) && match.empty()) {
    // Ideally we won't be calling this with an empty match, but just in case we error out.
    luaL_error(L, "invalid argument");
  }

  if (!requestAccess(permission, match)) {
    const auto& permissionString = permission_to_string(permission);
    luaL_error(L,
               Strings::VFormat(
                 fmt::format("script_access.error_{}", permissionString).c_str(),
                 fmt::make_format_args(m_extensionName.empty() ? Strings::script_access_script() :
                                                                 Strings::script_access_extension(),
                                       match))
                 .c_str());
  }
}

void Engine::setDebugger(Debugger* debugger)
{
  m_debugger = debugger;
  if (m_debugger)
    lua_sethook(L, &wrap_hook<&Engine::lua_hook>, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);
  else
    lua_sethook(L, nullptr, 0, 0);
}

AppEvents* Engine::appEvents()
{
  if (!m_appEvents)
    m_appEvents = std::make_unique<AppEvents>(L);

  return m_appEvents.get();
}

WindowEvents* Engine::windowEvents(ui::Window* window)
{
  if (!m_windowEvents)
    m_windowEvents = std::make_unique<WindowEvents>(L, window);

  return m_windowEvents.get();
}

SpriteEvents* Engine::spriteEvents(const Sprite* sprite)
{
  auto it = m_spriteEvents.find(sprite->id());
  if (it != m_spriteEvents.end())
    return it->second.get();

  auto* spriteEvents = new SpriteEvents(L, sprite);
  auto id = sprite->id();
  m_spriteEvents[id].reset(spriteEvents);

  spriteEvents->SpriteClosed.connect([this, id] {
    auto it = m_spriteEvents.find(id);
    if (it != m_spriteEvents.end()) {
      // We cannot delete the SpriteEvent here because we are
      // iterating the same SpriteClosed connection of this
      // SpriteEvent.
      m_deletedSpriteEvents.push_back(std::move(it->second));
      m_spriteEvents.erase(it);

      ui::execute_from_ui_thread([this] { m_deletedSpriteEvents.clear(); });
    }
  });

  return spriteEvents;
}

bool Engine::evalCode(const std::string& code, const std::string& name)
{
  bool ok = true;
  try {
    if (code.substr(0, 4) == "\x1bLua" && !requestAccess(Permission::Bytecode))
      return false;

    if (luaL_loadbuffer(L, code.c_str(), code.size(), name.c_str()) || lua_pcall(L, 0, 1, 0)) {
      const char* s = lua_tostring(L, -1);
      if (s)
        ConsoleError(s);
      ok = false;
      m_returnCode = -1;
    }
    else {
      // Return code
      if (lua_isinteger(L, -1))
        m_returnCode = lua_tointeger(L, -1);
      else
        m_returnCode = 0;

      // Code was executed correctly
      if (m_printEvalResult && !lua_isnone(L, -1)) {
        if (const char* result = lua_tostring(L, -1))
          ConsolePrint(result);
      }
    }
    lua_pop(L, 1);
  }
  catch (const std::exception& ex) {
    handleException(ex);
    ok = false;
    m_returnCode = -1;
  }

  // Collect script garbage.
  lua_gc(L, LUA_GCCOLLECT);
  return ok;
}

bool Engine::hasLingeringObjects()
{
  if (m_objectTracker > 0)
    return true;

  if (!m_spriteEvents.empty()) {
    for (const auto& [sprite, events] : m_spriteEvents)
      if (!events->empty())
        return true;
  }

  return (m_appEvents && !m_appEvents->empty()) || (m_windowEvents && !m_windowEvents->empty());
}

void Engine::handleException(const std::exception& ex)
{
  luaL_where(L, 1);
  const char* where = lua_tostring(L, -1);
  luaL_traceback(L, L, ex.what(), 1);
  const char* traceback = lua_tostring(L, -1);
  const std::string msg(fmt::format("{}{}", where, traceback));
  lua_pop(L, 2);

  ConsoleError(msg);
}

bool Engine::evalFile(const std::string& filename, const Params& params)
{
  std::stringstream buf;
  {
    const std::ifstream s(FSTREAM_PATH(filename));
    // Returns false if we cannot open the file
    if (!s)
      return false;
    buf << s.rdbuf();
  }
  const std::string& absFilename = base::get_absolute_path(filename);

  const PackagePath path(L, absFilename, m_scriptStack);
  set_app_params(L, params);

  const bool result = evalCode(buf.str(), "@" + absFilename);
  return result;
}

bool Engine::evalUserFile(const std::string& filename, const Params& params)
{
  m_baseScript = filename;

  if (m_extensionName.empty() &&
      !App::instance()->preferences().developer.disableIntegrityCheck()) {
    // TODO: Integrity checks for Extensions (signing)
    auto* storage = PermissionStorage::instance();
    if (!storage->passesIntegrityCheck(filename))
      storage->reset(filename);
  }

  return evalFile(filename, params);
}

bool Engine::evalExtension(const std::string& entryPoint, const std::string& extensionName)
{
  m_extensionName = extensionName;
  return evalUserFile(entryPoint);
}

} // namespace app::script
