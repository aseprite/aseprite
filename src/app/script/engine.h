// Aseprite
// Copyright (C) 2018-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPTING_H_INCLUDED
#define APP_SCRIPTING_H_INCLUDED
#pragma once
#include <unordered_set>

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/commands/params.h"
#include "app/extensions.h"
#include "app/script/engine_functions.h"
#include "app/script/permissions.h"
#include "base/chrono.h"
#include "doc/object_ids.h"
#include "doc/user_data.h"

#include <map>
#include <stack>
#include <string>

struct lua_State;
struct lua_Debug;

namespace app::script {

class AppEvents;
class WindowEvents;
class SpriteEvents;
class Debugger;

class Engine final {
public:
  static constexpr auto kExtensionPrefix = "ext:";
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
  // Same as evalUserFile but marks this Engine as used by an Extension, which has a few different
  // properties and error messages, especially when related to permissions.
  bool evalExtension(const std::string& entryPoint, const std::string& extensionName);
  bool requestAccess(Permission permission, const std::string& match = std::string());

  // Reads the permission from storage and longjmps with an error if it's not given.
  void accessGate(Permission permission, const std::string& match = std::string());

  // Checks if the Engine has any tracked objects or attached events
  bool hasLingeringObjects();

  void handleException(const std::exception& ex);

  void lua_hook(lua_Debug* ar) const;

  obs::signal<void(const std::string&)> ConsolePrint;
  obs::signal<void(const std::string&)> ConsoleError;

private:
  // Functions registered directly to Lua:
  int lua_dofile();
  int lua_loadfile();
  int lua_print();
  int lua_io_input();
  int lua_io_lines();
  int lua_io_open();
  int lua_io_output();
  int lua_io_popen();
  int lua_os_clock();
  int lua_os_tmpname();
  int lua_os_execute();
  int lua_os_remove();
  int lua_os_rename();
  int lua_package_loadlib();

  // Helper to run similar code for IO access to files
  int absoluteFilenameAccess(lua_CFunction func, bool readOnly = true);

  lua_State* L;
  Debugger* m_debugger = nullptr;

  // Events
  std::unique_ptr<AppEvents> m_appEvents;
  std::unique_ptr<WindowEvents> m_windowEvents;
  std::map<doc::ObjectId, std::unique_ptr<SpriteEvents>> m_spriteEvents;
  std::vector<std::unique_ptr<SpriteEvents>> m_deletedSpriteEvents;

  std::unordered_set<std::string> m_temporaryFiles;

  // Holds the base "entry point" of this engine, the last filename with which evalUserFile was
  // called, remains after execution has finished, for any events/dialogs that might need to run
  // relative-path scripts.
  std::string m_baseScript;

  std::string m_extensionName;

  // Stack of script filenames that are being executed.
  std::stack<std::string> m_scriptStack;

  MemoryTracker m_tracker;
  base::Chrono m_clock;
  bool m_printEvalResult;
  int m_returnCode;
  uint32_t m_objectTracker;
};

// Used by App.open(), Sprite{ fromFile }, and Image{ fromFile }
enum class LoadSpriteFromFileParam : uint8_t { FullAniAsSprite, OneFrameAsSprite, OneFrameAsImage };
int load_sprite_from_file(lua_State* L, const char* filename, LoadSpriteFromFileParam param);

Engine* get_engine(lua_State* L);
void engine_print(lua_State* L, const std::string& message);
lua_CFunction engine_io_open();

} // namespace app::script

#endif
