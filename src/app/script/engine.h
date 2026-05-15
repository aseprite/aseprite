// Aseprite
// Copyright (C) 2018-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPTING_H_INCLUDED
#define APP_SCRIPTING_H_INCLUDED
#pragma once

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/color.h"
#include "app/commands/params.h"
#include "app/extensions.h"
#include "base/chrono.h"
#include "base/time.h"
#include "base/uuid.h"
#include "doc/brush.h"
#include "doc/frame.h"
#include "doc/object_ids.h"
#include "doc/pixel_format.h"
#include "doc/tile.h"
#include "doc/user_data.h"
#include "gfx/fwd.h"

#include <map>
#include <stack>
#include <string>

struct lua_State;
struct lua_Debug;

namespace base {
class Version;
}

namespace gfx {
class ColorSpace;
}

namespace doc {
class Cel;
class Image;
class Layer;
class Mask;
class Palette;
class Sprite;
class Tag;
class Tileset;
class Tilesets;
class WithUserData;
} // namespace doc

namespace ui {
class Window;
}

namespace app {

class Editor;
class Site;

namespace tools {
class Tool;
}

namespace script {

class AppEvents;
class WindowEvents;
class SpriteEvents;
class Debugger;

class Engine {
public:
  struct MemoryTracker {
    size_t initialUsage = 0;
    size_t usage = 0;
    size_t limit = 256'000'000;
  };

  Engine();
  ~Engine();

  void setPrintEvalResult(const bool printEvalResult) { m_printEvalResult = printEvalResult; }
  void setMemoryLimit(const size_t& limit) { m_tracker.limit = limit; }
  int returnCode() const { return m_returnCode; }
  lua_State* luaState() const { return L; }
  const MemoryTracker& memoryTracker() const { return m_tracker; }
  void setDebugger(Debugger* debugger);
  AppEvents* appEvents();
  WindowEvents* windowEvents(ui::Window* window);
  SpriteEvents* spriteEvents(const doc::Sprite* sprite);

  // Adds an object to the tracker used by hasLingeringObject(), used for Dialogs, Timers and
  // anything that might linger after execution.
  void trackObject() { m_objectTracker++; }
  void untrackObject() { m_objectTracker--; }

  bool evalCode(const std::string& code, const std::string& name = std::string());
  bool evalFile(const std::string& filename, const Params& params = Params());
  bool evalUserFile(const std::string& filename, const Params& params = Params());

  // Checks if the Engine has any tracked objects or attached events
  bool hasLingeringObjects();

  void handleException(const std::exception& ex);

  // Functions registered directly to Lua:
  int lua_print();
  int lua_dofile();
  int lua_loadfile();
  int lua_os_clock();
  void lua_hook(lua_Debug* ar);

  obs::signal<void(const std::string&)> ConsolePrint;
  obs::signal<void(const std::string&)> ConsoleError;

private:
  lua_State* L;
  Debugger* m_debugger = nullptr;

  // Events
  std::unique_ptr<AppEvents> m_appEvents;
  std::unique_ptr<WindowEvents> m_windowEvents;
  std::map<doc::ObjectId, std::unique_ptr<SpriteEvents>> m_spriteEvents;

  // Holds the base "entry point" of this engine, the last filename with which evalUserFile was
  // called, remains after execution has finished, for any events/dialogs that might need to run
  // relative-path scripts.
  std::string m_baseScript;

  // Stack of script filenames that are being executed.
  std::stack<std::string> m_scriptStack;

  MemoryTracker m_tracker;
  base::Chrono m_clock;
  bool m_printEvalResult;
  int m_returnCode;
  uint32_t m_objectTracker;
};

int ObjectIterator_pairs_next(lua_State* L);

void push_app_events(lua_State* L);
void push_app_theme(lua_State* L, int uiscale = 1);
void push_app_clipboard(lua_State* L);
int push_image_iterator_function(lua_State* L, const doc::Image* image, int extraArgIndex);
void push_brush(lua_State* L, const doc::BrushRef& brush);
void push_cel_image(lua_State* L, doc::Cel* cel);
void push_cel_images(lua_State* L, const doc::ObjectIds& cels);
void push_cels(lua_State* L, const doc::ObjectIds& cels);
void push_cels(lua_State* L, doc::Layer* layer);
void push_cels(lua_State* L, doc::Sprite* sprite);
void push_color_space(lua_State* L, const gfx::ColorSpace& cs);
void push_doc_range(lua_State* L, Site& site);
void push_editor(lua_State* L, Editor* editor);
void push_group_layers(lua_State* L, doc::Layer* group);
void push_image(lua_State* L, doc::Image* image);
void push_layers(lua_State* L, const doc::ObjectIds& layers);
void push_palette(lua_State* L, doc::Palette* palette);
void push_plugin(lua_State* L, Extension* ext);
void push_properties(lua_State* L, doc::WithUserData* userData, const std::string& extID);
void push_sprite_cel(lua_State* L, doc::Cel* cel);
void push_sprite_events(lua_State* L, doc::Sprite* sprite);
void push_sprite_frame(lua_State* L, doc::Sprite* sprite, doc::frame_t frame);
void push_sprite_frames(lua_State* L, doc::Sprite* sprite);
void push_sprite_frames(lua_State* L, doc::Sprite* sprite, const std::vector<doc::frame_t>& frames);
void push_sprite_layers(lua_State* L, doc::Sprite* sprite);
void push_sprite_palette(lua_State* L, doc::Sprite* sprite, doc::Palette* palette);
void push_sprite_palettes(lua_State* L, doc::Sprite* sprite);
void push_sprite_selection(lua_State* L, doc::Sprite* sprite);
void push_sprite_slices(lua_State* L, doc::Sprite* sprite);
void push_sprite_tags(lua_State* L, doc::Sprite* sprite);
void push_sprites(lua_State* L);
void push_standalone_selection(lua_State* L, doc::Mask* mask);
void push_tile(lua_State* L, const doc::Tileset* tileset, doc::tile_index ti);
void push_tile_properties(lua_State* L,
                          const doc::Tileset* tileset,
                          doc::tile_index ti,
                          const std::string& extID);
void push_tileset(lua_State* L, const doc::Tileset* tileset);
void push_tileset_image(lua_State* L, doc::Tileset* tileset, doc::tile_index ti);
void push_tilesets(lua_State* L, doc::Tilesets* tilesets);
void push_tool(lua_State* L, app::tools::Tool* tool);
void push_version(lua_State* L, const base::Version& ver);
void push_window_events(lua_State* L, ui::Window* window);

gfx::Point convert_args_into_point(lua_State* L, int index);
gfx::Rect convert_args_into_rect(lua_State* L, int index);
gfx::Size convert_args_into_size(lua_State* L, int index);
app::Color convert_args_into_color(lua_State* L, int index);
doc::color_t convert_args_into_pixel_color(lua_State* L, int index, doc::PixelFormat pixelFormat);
base::Uuid convert_args_into_uuid(lua_State* L, int index);
doc::Palette* get_palette_from_arg(lua_State* L, int index);
doc::Image* may_get_image_from_arg(lua_State* L, int index);
doc::Image* get_image_from_arg(lua_State* L, int index);
doc::Cel* get_image_cel_from_arg(lua_State* L, int index);
doc::Tileset* get_image_tileset_from_arg(lua_State* L, int index);
doc::frame_t get_frame_number_from_arg(lua_State* L, int index);
doc::Mask* get_mask_from_arg(lua_State* L, int index);
app::tools::Tool* get_tool_from_arg(lua_State* L, int index);
doc::BrushRef get_brush_from_arg(lua_State* L, int index);
doc::Tileset* get_tile_index_from_arg(lua_State* L, int index, doc::tile_index& ts);
doc::UserData::Properties* may_get_properties(lua_State* L, int index);

// Used by App.open(), Sprite{ fromFile }, and Image{ fromFile }
enum class LoadSpriteFromFileParam : uint8_t { FullAniAsSprite, OneFrameAsSprite, OneFrameAsImage };
int load_sprite_from_file(lua_State* L, const char* filename, LoadSpriteFromFileParam param);

Engine* get_engine(lua_State* L);
void engine_print(lua_State* L, const std::string& message);

} // namespace script
} // namespace app

#endif
