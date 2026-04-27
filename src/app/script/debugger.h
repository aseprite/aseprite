// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_DEBUGGER_H_INCLUDED
#define APP_SCRIPT_DEBUGGER_H_INCLUDED
#pragma once

#ifdef ENABLE_SCRIPTING
  #include "lua.h"

  #include "obs/signal.h"
  #include "ui/register_message.h"

  #include <map>
  #include <vector>

struct lua_State;
struct lua_Debug;

namespace dap {
class integer;
struct Variable;
struct StackFrame;
namespace net {
class Server;
}
class ReaderWriter;
class Session;
} // namespace dap

namespace app {
class Extension;
}

namespace app::script {
class Engine;

extern ui::RegisterMessage kDebuggerWakeMessage;

class Debugger final {
  friend Extension;
  enum class State : uint8_t { Pass, Wait, NextLine, StepIn, StepOut, Abort };

  struct VariableReference {
    std::string name;
    size_t index = 0;
    int ref = -1;
  };

public:
  explicit Debugger(const std::string& address, uint16_t port);
  ~Debugger();

  void setHookState(State hookState);
  bool isWaiting() const { return m_hookState.load() == State::Wait; }
  bool isDebugging() const { return (m_engine || m_extension); }

  void onHook(lua_State* L, lua_Debug* ar);
  void onExtensionChange(Extension* ext);

  obs::safe_signal<void(const std::string&)> Error;

protected:
  void onClientConnected(const std::shared_ptr<dap::ReaderWriter>& rw);

private:
  void updateVariables(lua_State* L);
  void updateStackTrace(lua_State* L);
  dap::Variable makeVariable(lua_State* L, const std::string& name);
  void stop(const std::string& reason, dap::integer line);

  obs::scoped_connection m_changeConn;

  std::unique_ptr<dap::net::Server> m_server;
  std::unique_ptr<dap::Session> m_session;

  std::mutex m_mutex;
  std::multimap<std::string, int> m_breakpoints;
  std::unique_ptr<Engine> m_engine;
  Extension* m_extension;
  std::atomic<State> m_hookState;

  // The currently executing stack level of the Lua script
  uint32_t m_hookStackLevel;
  // The stack level that we want to step through.
  uint32_t m_commandStackLevel;

  std::vector<dap::StackFrame> m_currentStackTrace;
  std::vector<dap::Variable> m_locals;
  std::vector<dap::Variable> m_globals;

  std::vector<std::vector<VariableReference>> m_registry;
};

} // namespace app::script

#endif // ENABLE_SCRIPTING

#endif
