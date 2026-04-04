// Aseprite
// Copyright (C) 2018-2024  Igara Studio S.A.
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
#include "app/pref/preferences.h"
#include "app/script/blend_mode.h"
#include "app/script/luacpp.h"
#include "app/script/security.h"
#include "app/sprite_sheet_type.h"
#include "app/tilemap_mode.h"
#include "app/tileset_mode.h"
#include "app/tools/ink_type.h"
#include "base/chrono.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/fstream_path.h"
#include "doc/algorithm/flip_type.h"
#include "doc/anidir.h"
#include "doc/color_mode.h"
#include "events.h"
#include "filters/target.h"
#include "fmt/format.h"
#include "ui/base.h"
#include "ui/cursor_type.h"
#include "ui/mouse_button.h"

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

namespace {
int dofilecont(lua_State* L, int d1, lua_KContext d2)
{
  (void)d1;
  (void)d2;
  return lua_gettop(L) - 1;
}
} // namespace

Engine* get_engine(lua_State* L)
{
  auto* ptr = lua_getextraspace(L);
  return *static_cast<Engine**>(ptr);
}

void engine_print(lua_State* L, const std::string& message)
{
  get_engine(L)->ConsolePrint(message);
}

void register_app_object(lua_State* L);
void register_app_pixel_color_object(lua_State* L);
void register_app_fs_object(lua_State* L);
void register_app_os_object(lua_State* L);
void register_app_command_object(lua_State* L);
void register_app_preferences_object(lua_State* L);
void register_json_object(lua_State* L);

void register_brush_class(lua_State* L);
void register_cel_class(lua_State* L);
void register_cels_class(lua_State* L);
void register_color_class(lua_State* L);
void register_color_space_class(lua_State* L);
void register_dialog_class(lua_State* L);
void register_editor_class(lua_State* L);
void register_graphics_context_class(lua_State* L);
void register_window_class(lua_State* L);
void register_events_class(lua_State* L);
void register_frame_class(lua_State* L);
void register_frames_class(lua_State* L);
void register_grid_class(lua_State* L);
void register_image_class(lua_State* L);
void register_image_iterator_class(lua_State* L);
void register_image_spec_class(lua_State* L);
void register_images_class(lua_State* L);
void register_layer_class(lua_State* L);
void register_layers_class(lua_State* L);
void register_palette_class(lua_State* L);
void register_palettes_class(lua_State* L);
void register_plugin_class(lua_State* L);
void register_point_class(lua_State* L);
void register_properties_class(lua_State* L);
void register_range_class(lua_State* L);
void register_rect_class(lua_State* L);
void register_selection_class(lua_State* L);
void register_site_class(lua_State* L);
void register_size_class(lua_State* L);
void register_slice_class(lua_State* L);
void register_slices_class(lua_State* L);
void register_sprite_class(lua_State* L);
void register_sprites_class(lua_State* L);
void register_tag_class(lua_State* L);
void register_tags_class(lua_State* L);
void register_theme_classes(lua_State* L);
void register_clipboard_classes(lua_State* L);
void register_tile_class(lua_State* L);
void register_tileset_class(lua_State* L);
void register_tilesets_class(lua_State* L);
void register_timer_class(lua_State* L);
void register_tool_class(lua_State* L);
void register_uuid_class(lua_State* L);
void register_version_class(lua_State* L);
void register_websocket_class(lua_State* L);

void set_app_params(lua_State* L, const Params& params);

namespace {

// Wraps member functions to be registered directly to Lua.
using memberFunction = int (Engine::*)();
template<memberFunction function>
int wrap(lua_State* L)
{
  Engine* ptr = *static_cast<Engine**>(lua_getextraspace(L));
  return (ptr->*function)();
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
        "Script memory limit hit: Using %d, attempted to allocate %d.\n",
        tracker->usage,
        nsize);
    return nullptr;
  }
  tracker->usage += nsize;
  if (ptr != nullptr)
    tracker->usage -= osize;
  return base_realloc(ptr, nsize);
}

struct PackagePath {
  std::string previousPath;
  std::stack<std::string>& stack;
  lua_State* L;
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

lua_CFunction g_orig_loadfile = nullptr;

// Equivalent to what's declared  luaL_openlibs except without coroutines.
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

constexpr auto registration_functions = {
  register_app_object,
  register_app_pixel_color_object,
  register_app_fs_object,
  register_app_os_object,
  register_app_command_object,
  register_app_preferences_object,
  register_json_object,
  register_brush_class,
  register_cel_class,
  register_cels_class,
  register_color_class,
  register_color_space_class,
  register_dialog_class,
  register_editor_class,
  register_graphics_context_class,
  register_window_class,
  register_events_class,
  register_frame_class,
  register_frames_class,
  register_grid_class,
  register_image_class,
  register_image_iterator_class,
  register_image_spec_class,
  register_images_class,
  register_layer_class,
  register_layers_class,
  register_palette_class,
  register_palettes_class,
  register_plugin_class,
  register_point_class,
  register_properties_class,
  register_range_class,
  register_rect_class,
  register_selection_class,
  register_site_class,
  register_size_class,
  register_slice_class,
  register_slices_class,
  register_sprite_class,
  register_sprites_class,
  register_tag_class,
  register_tags_class,
  register_theme_classes,
  register_clipboard_classes,
  register_tile_class,
  register_tileset_class,
  register_tilesets_class,
  register_timer_class,
  register_tool_class,
  register_uuid_class,
  register_version_class,
#if ENABLE_WEBSOCKET
  register_websocket_class,
#endif
};
} // namespace

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
    lua_pop(L, 1); /* remove lib */
  }

  // Secure Lua functions
  overwrite_unsecure_functions(L);

  // Overwrite Lua functions with custom implementations
  lua_register(L, "print", &wrap<&Engine::lua_print>);
  lua_register(L, "dofile", &wrap<&Engine::lua_dofile>);
  if (!g_orig_loadfile) {
    lua_getglobal(L, "loadfile");
    g_orig_loadfile = lua_tocfunction(L, -1);
    lua_pop(L, 1);
  }
  lua_register(L, "loadfile", &wrap<&Engine::lua_loadfile>);

  // TODO: Do we want a global clock or are we keeping one per engine?
  lua_getglobal(L, "os");
  lua_pushcfunction(L, &wrap<&Engine::lua_os_clock>);
  lua_setfield(L, -2, "clock");
  lua_pop(L, 1);

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
  for (const auto fn : registration_functions) {
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
  lua_close(L);
  L = nullptr;
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

int Engine::lua_dofile()
{
  const char* argFname = luaL_optstring(L, 1, NULL);
  std::string fname = argFname;

  if (!base::is_file(fname)) {
    const auto top = m_scriptStack.empty() ? m_baseScript : m_scriptStack.top();

    // Try to complete a relative filename
    const std::string altFname = base::join_path(base::get_file_path(top), fname);
    if (base::is_file(altFname))
      fname = altFname;
  }

  lua_settop(L, 1);
  if (luaL_loadfile(L, fname.c_str()) != LUA_OK)
    return lua_error(L);
  {
    const PackagePath set(L, fname, m_scriptStack);
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
    return luaL_error(L, "loadfile() for stdin cannot be used running in GUI mode");
  }
  return g_orig_loadfile(L);
}

int Engine::lua_os_clock()
{
  lua_pushnumber(L, m_clock.elapsed());
  return 1;
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
  spriteEvents->SpriteClosed.connect([this, id] { m_spriteEvents.erase(id); });
  return spriteEvents;
}

bool Engine::evalCode(const std::string& code, const std::string& name)
{
  bool ok = true;
  try {
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
    std::ifstream s(FSTREAM_PATH(filename));
    // Returns false if we cannot open the file
    if (!s)
      return false;
    buf << s.rdbuf();
  }
  const std::string& absFilename = base::get_absolute_path(filename);

  const PackagePath path(L, absFilename, m_scriptStack);
  set_app_params(L, params);

  bool result = evalCode(buf.str(), "@" + absFilename);
  return result;
}

bool Engine::evalUserFile(const std::string& filename, const Params& params)
{
  m_baseScript = filename;
  return evalFile(filename, params);
}

} // namespace app::script
