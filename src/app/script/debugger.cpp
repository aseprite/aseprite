// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/resource_finder.h"
#include "app/script/debugger.h"
#include "app/script/docobj.h"
#include "app/script/engine_manager.h"
#include "app/script/luacpp.h"
#include "base/exception.h"
#include "base/fs.h"
#include "doc/layer.h"
#include "ui/manager.h"
#include "ui/message_loop.h"
#include "ui/system.h"

#include "dap/io.h"
#include "dap/network.h"
#include "dap/protocol.h"
#include "dap/session.h"
#include "fmt/format.h"

#include <future>

// Arbitrary "main thread" ID for cppdap
#define DBG_THREAD_ID 0451

struct AseLaunchRequest : dap::LaunchRequest {
  using Response = dap::LaunchResponse;
  dap::optional<dap::string> extensionName;
  dap::optional<dap::string> scriptFilename;
};

struct AseAttachRequest : dap::AttachRequest {
  using Response = dap::AttachResponse;
  dap::optional<dap::string> extensionName;
};

namespace {
std::string next_key_string(lua_State* L)
{
  lua_pushvalue(L, -2);
  std::string key;
  const int keyType = lua_type(L, -1);
  if (keyType == LUA_TSTRING || keyType == LUA_TNUMBER) {
    key = lua_tostring(L, -1);
  }
  else if (keyType == LUA_TBOOLEAN) {
    key = fmt::format("[bool:{}]", lua_toboolean(L, -1) ? "true" : "false");
  }
  else {
    key = fmt::format("[{}]", lua_typename(L, keyType));
  }
  lua_pop(L, 1);
  return key;
}
} // namespace

namespace dap {
DAP_STRUCT_TYPEINFO_EXT(AseLaunchRequest,
                        LaunchRequest,
                        "launch",
                        DAP_FIELD(extensionName, "extensionName"),
                        DAP_FIELD(scriptFilename, "scriptFilename"));
DAP_STRUCT_TYPEINFO_EXT(AseAttachRequest,
                        AttachRequest,
                        "attach",
                        DAP_FIELD(extensionName, "extensionName"));
} // namespace dap

namespace app::script {

ui::RegisterMessage kDebuggerWakeMessage;

Debugger::Debugger(const std::string& address, const uint16_t port)
  : m_extension(nullptr)
  , m_hookState(State::Pass)
  , m_hookStackLevel(0)
  , m_commandStackLevel(0)
{
  m_server = dap::net::Server::create();
  const bool result = m_server->start(
    address.c_str(),
    port,
    [this](const auto& rw) { onClientConnected(rw); },
    [this](const char* msg) {
      Error(fmt::format("Debugger error: {}", msg));
      m_session.reset();
    });
  if (!result)
    throw base::Exception("Could not create debugger server.");

  m_changeConn = App::instance()->extensions().ScriptsChange.connect(&Debugger::onExtensionChange,
                                                                     this);
}

Debugger::~Debugger()
{
  ASSERT(!isDebugging());
  m_session.reset();
  m_server->stop();
}

void Debugger::setHookState(const State hookState)
{
  if (m_hookState == hookState)
    return;

  m_hookState = hookState;

  if (hookState == State::Wait)
    return;

  // When we're going from a wait state to another, we need to send a message to the queue wake up
  // the loop inside the hook
  ui::execute_from_ui_thread(
    [] { ui::Manager::getDefault()->enqueueMessage(new ui::Message(kDebuggerWakeMessage)); });
}

void Debugger::updateVariables(lua_State* L)
{
  // Go through the registry and unref all the refs.
  for (const auto& refs : m_registry) {
    for (const auto& ref : refs) {
      luaL_unref(L, LUA_REGISTRYINDEX, ref.ref);
    }
  }
  m_registry.clear();

  // Updating locals
  m_locals.clear();
  lua_Debug ar;
  if (lua_getstack(L, 0, &ar)) {
    for (int n = 1;; ++n) {
      const char* name = lua_getlocal(L, &ar, n);
      if (!name)
        break;

      // These special names are returned by luaG_findlocal()
      if (strcmp(name, "(temporary)") == 0 || strcmp(name, "(C temporary)") == 0) {
        lua_pop(L, 1);
        continue;
      }

      m_locals.push_back(makeVariable(L, name));
      lua_pop(L, 1);
    }
  }

  // Updating globals
  m_globals.clear();
  lua_pushglobaltable(L);
  lua_pushnil(L);
  while (lua_next(L, -2) != 0) {
    const std::string name = next_key_string(L);
    if (name != "_G" && name != "_ENV")
      m_globals.push_back(makeVariable(L, name));
    lua_pop(L, 1);
  }
  lua_pop(L, 1);
}

void Debugger::updateStackTrace(lua_State* L)
{
  m_currentStackTrace.clear();
  lua_Debug ar;
  int level = 0;
  while (lua_getstack(L, level++, &ar)) {
    dap::StackFrame frame;
    frame.id = level;
    lua_getinfo(L, "Slntf", &ar);
    const std::string filename(ar.short_src, ar.srclen - 1);

    if (!filename.empty() && filename != "[C]") {
      dap::Source source;
      source.path = filename;
      frame.source = source;
    }

    if (ar.currentline > 0)
      frame.line = ar.currentline;

    // If we're inside a pcall we lose ar.name
    if (ar.name) {
      frame.name = ar.name;
    }
    else if (strcmp(ar.what, "main") == 0) {
      frame.name = "[main]";
    }
    else {
      frame.name = "[unnamed function]";
    }

    if (ar.istailcall)
      frame.name += " (tailcall)";
    m_currentStackTrace.push_back(frame);

    lua_pop(L, 1);
  }
}

dap::Variable Debugger::makeVariable(lua_State* L, const std::string& name)
{
  dap::Variable var;
  var.name = name;

  switch (lua_type(L, -1)) {
    case LUA_TNIL:
      var.type = "nil";
      var.value = "nil";
      break;
    case LUA_TBOOLEAN:
      var.type = "boolean";
      var.value = lua_toboolean(L, -1) ? "true" : "false";
      break;
    case LUA_TNUMBER:
      var.type = "number";
      if (lua_isinteger(L, -1)) {
        var.value = std::to_string(lua_tointeger(L, -1));
      }
      else {
        var.value = std::to_string(lua_tonumber(L, -1));
      }
      break;
    case LUA_TSTRING:
      var.type = "string";
      if (auto* str = lua_tostring(L, -1))
        var.value = str;
      break;
    case LUA_TLIGHTUSERDATA: var.type = "lightuserdata"; break;
    case LUA_TFUNCTION:      var.type = "function"; break;
    case LUA_TTABLE:         {
      var.type = "table";
      lua_pushnil(L);
      std::vector<VariableReference> references;
      while (lua_next(L, -2) != 0) {
        VariableReference ref;
        ref.name = next_key_string(L);
        ref.ref = luaL_ref(L, LUA_REGISTRYINDEX);
        references.push_back(ref);
      }
      var.namedVariables = references.size();
      m_registry.push_back(references);
      var.variablesReference = m_registry.size();
      break;
    }
    case LUA_TUSERDATA: {
      if (luaL_getmetafield(L, -1, "__typename") == LUA_TSTRING) {
        if (const auto* typeName = lua_tostring(L, -1))
          var.type = typeName;
        lua_pop(L, 1);
      }

      if (luaL_getmetafield(L, -1, "__pairs") == LUA_TFUNCTION) {
        std::vector<VariableReference> references;
        lua_pushvalue(L, -2);
        lua_call(L, 1, 3);
        for (;;) {
          lua_pushvalue(L, -2);
          lua_pushvalue(L, -2);
          lua_copy(L, -5, -3);
          lua_call(L, 2, 2);
          const int type = lua_type(L, -2);
          if (type == LUA_TNIL) {
            lua_pop(L, 4);
            break;
          }
          VariableReference ref;
          ref.name = next_key_string(L);
          ref.ref = luaL_ref(L, LUA_REGISTRYINDEX);
          references.push_back(ref);
        }

        m_registry.push_back(references);
        var.indexedVariables = references.size();
        var.variablesReference = m_registry.size();
      }
      else if (luaL_getmetafield(L, -1, "__len") == LUA_TFUNCTION) {
        // There doesn't seem to be a way to check for the __len metamethod without also putting it
        // in the stack and we *could* call it manually, but it's more finicky than using lua_len
        // so we just pop the function back out.
        lua_pop(L, 1);

        std::vector<VariableReference> references;
        auto objIndex = lua_absindex(L, -1);
        lua_len(L, objIndex);
        auto len = lua_tointeger(L, -1);
        lua_pop(L, 1);

        for (lua_Integer i = 1; i <= len; i++) {
          lua_pushinteger(L, i);
          lua_gettable(L, objIndex);
          if (!lua_isnil(L, -1)) {
            VariableReference ref;
            ref.index = i;
            ref.ref = luaL_ref(L, LUA_REGISTRYINDEX);
            references.push_back(ref);
          }
          else
            lua_pop(L, 1);
        }
        m_registry.push_back(references);
        var.variablesReference = m_registry.size();
        if (!var.type.has_value())
          var.type = "table";
      }
      else {
        // Last ditch effort, stringify the value
        size_t len;
        const char* str = luaL_tolstring(L, -1, &len);
        lua_pop(L, 1);
        if (len > 0) {
          var.type = "string";
          var.value = std::string(str, len);
        }
      }

      if (!var.type.has_value())
        var.type = "userdata";
      break;
    }
    case LUA_TTHREAD: var.type = "thread"; break;
    default:          break;
  }

  return var;
}

// Sends the stop event and sets the hook state to "wait"
// Valid reasons:
// 'step', 'breakpoint', 'exception', 'pause', 'entry', 'goto', 'function breakpoint',
// 'data breakpoint', 'instruction breakpoint'
void Debugger::stop(const std::string& reason, const dap::integer line)
{
  dap::StoppedEvent event;
  event.reason = reason;
  event.threadId = DBG_THREAD_ID;
  m_session->send(event);
  setHookState(State::Wait);
}

void Debugger::onHook(lua_State* L, lua_Debug* ar)
{
  ASSERT(isDebugging());
  ASSERT(m_session);
  ASSERT(ar);

  if (m_hookState.load() == State::Abort)
    return;

  const int ret = lua_getinfo(L, "lSn", ar);
  if (ret == 0 || ar->currentline < 0)
    return;

  switch (ar->event) {
    case LUA_HOOKCALL: ++m_hookStackLevel; break;
    case LUA_HOOKRET:  --m_hookStackLevel; break;
    case LUA_HOOKLINE:
      if (m_hookState.load() == State::NextLine && m_hookStackLevel <= m_commandStackLevel) {
        m_commandStackLevel = m_hookStackLevel;
        stop("step", ar->currentline);
        break;
      }

      std::string filename = ar->source;
      if (!filename.empty() && filename[0] == '@')
        filename = ar->source + 1;
      filename = base::normalize_path(filename);
#ifdef LAF_WINDOWS
      filename = base::string_to_lower(filename);
#endif

      const std::lock_guard lock(m_mutex);
      for (auto [itr, rangeEnd] = m_breakpoints.equal_range(filename); itr != rangeEnd; ++itr) {
        if (itr->second == ar->currentline) {
          stop("breakpoint", ar->currentline);
          break;
        }
      }
      break;
  }

  if (m_hookState.load() == State::Wait) {
    {
      const std::lock_guard lock(m_mutex);
      updateVariables(L);
      updateStackTrace(L);
    }
    ui::MessageLoop loop(ui::Manager::getDefault());
    while (m_hookState.load() == State::Wait)
      loop.pumpMessages();
  }
}

void Debugger::onExtensionChange(Extension* ext)
{
  if (!m_extension)
    return;

  if (m_extension == ext && !m_extension->isEnabled()) {
    // If scripts have changed in any way we must terminate the debugging session to avoid having a
    // dangling Extension pointer.
    m_session->send(dap::TerminatedEvent{});
  }
}

void Debugger::onClientConnected(const std::shared_ptr<dap::ReaderWriter>& rw)
{
  ASSERT(!isDebugging());

  // Clear everything before we allow another client.
  m_currentStackTrace.clear();
  m_breakpoints.clear();
  m_locals.clear();
  m_globals.clear();
  m_commandStackLevel = 0;
  m_hookStackLevel = 0;

  m_session = dap::Session::create();
  m_session->onError(
    [this](const char* msg) { Error(fmt::format("Debugger session error: {}", msg)); });

  m_session->registerHandler([](const dap::InitializeRequest&) {
    dap::InitializeResponse response;
    response.supportsReadMemoryRequest = false;
    response.supportsDataBreakpoints = false;
    response.supportsExceptionOptions = false;
    response.supportsDelayedStackTraceLoading = false;
    response.supportsSetVariable = false;
    response.supportsConditionalBreakpoints = false;
    response.supportsConfigurationDoneRequest = true;
    return response;
  });

  m_session->registerSentHandler([this](const dap::ResponseOrError<dap::InitializeResponse>&) {
    m_session->send(dap::InitializedEvent{});
  });

  m_session->registerHandler([](const dap::ThreadsRequest&) {
    dap::ThreadsResponse response;
    dap::Thread thread;
    thread.id = DBG_THREAD_ID;
    thread.name = "Main";
    response.threads.push_back(thread);
    return response;
  });

  m_session->registerHandler([this](const AseAttachRequest& request)
                               -> dap::ResponseOrError<dap::AttachResponse> {
    if (isDebugging())
      return dap::Error("Aseprite does not support multiple simultaneous debug sessions.");

    if (!request.extensionName.has_value() || request.extensionName.value().empty())
      return dap::Error("Requires the extensionName arguments");

    for (Extension* ext : App::instance()->extensions()) {
      if (ext->name() == request.extensionName.value()) {
        if (!ext->isEnabled())
          return dap::Error(
            "The Extension is not currently enabled, enable it inside Aseprite or use the 'launch' request.");

        if (!ext->hasScripts())
          return dap::Error("The Extension does not have any scripts to debug.");

        m_extension = ext;
        setHookState(State::Pass);
        ui::execute_from_ui_thread([this] { m_extension->scriptEngine()->setDebugger(this); });

        return dap::AttachResponse{};
      }
    }

    return dap::Error("Extension not found");
  });

  m_session->registerHandler(
    [this](const AseLaunchRequest& request) -> dap::ResponseOrError<dap::LaunchResponse> {
      if (request.noDebug)
        return dap::Error("'No debug' option is unsupported");

      if (isDebugging())
        return dap::Error("Aseprite does not support multiple simultaneous debug sessions.");

      if ((!request.extensionName.has_value() || request.extensionName.value().empty()) &&
          (!request.scriptFilename.has_value() || request.scriptFilename.value().empty()))
        return dap::Error("Requires the extensionName or scriptFilename arguments");

      if (request.scriptFilename.has_value() && !request.scriptFilename.value().empty()) {
        std::string filename = request.scriptFilename.value();

        if (!base::is_absolute_path(filename) || base::is_file(filename)) {
          ResourceFinder rf;
          rf.includeUserDir(base::join_path("scripts", filename).c_str());
          if (rf.findFirst())
            filename = rf.filename();
        }
        filename = base::normalize_path(filename);
        if (!base::is_file(filename))
          return dap::Error("Could not find scriptFilename");

        ui::execute_from_ui_thread([this, filename] {
          setHookState(State::Pass);
          m_engine = EngineManager::create();
          std::string lastError;
          m_engine->ConsoleError.connect(
            [&lastError](const std::string& error) { lastError = error; });
          m_engine->setDebugger(this);
          m_engine->evalUserFile(filename);

          if (m_engine->returnCode() < 0) {
            dap::StoppedEvent event;
            event.description = "Script encountered an error";
            event.reason = "exception";
            event.text = lastError;
            m_session->send(event);
          }

          m_engine.reset();
          if (m_hookState.load() != State::Abort)
            m_session->send(dap::TerminatedEvent{});
        });
      }
      else if (request.extensionName.has_value() && !request.extensionName.value().empty()) {
        for (Extension* ext : App::instance()->extensions()) {
          if (base::string_to_lower(ext->name()) ==
              base::string_to_lower(request.extensionName.value())) {
            if (!ext->hasScripts())
              return dap::Error("The Extension does not have any scripts to debug.");

            std::promise<Extension*> promise;
            ui::execute_from_ui_thread([this, &promise, ext] {
              if (ext->isEnabled())
                App::instance()->extensions().enableExtension(ext, false);

              setHookState(State::Pass);

              // Pre-initialize the engine with the debugger already created, so that we can
              // set breakpoints inside init().
              ext->m_engine = EngineManager::create();
              ext->m_engine->setDebugger(this);
              promise.set_value(ext);
            });

            m_extension = promise.get_future().get();

            ui::execute_from_ui_thread(
              [this] { App::instance()->extensions().enableExtension(m_extension, true); });
          }
        }
        if (!m_extension)
          return dap::Error(
            fmt::format("Could not find extension '{}'", request.extensionName.value()));
      }

      return dap::LaunchResponse{};
    });

  m_session->registerHandler([this](const dap::SetBreakpointsRequest& request)
                               -> dap::ResponseOrError<dap::SetBreakpointsResponse> {
    const std::lock_guard lock(m_mutex);

    if (!request.breakpoints.has_value())
      return dap::SetBreakpointsResponse{};

    if (!request.source.path.has_value())
      return dap::Error("Cannot find source for breakpoint");

    dap::SetBreakpointsResponse response{};

    auto path = base::normalize_path(request.source.path.value());
#ifdef LAF_WINDOWS
    // Some debuggers can send the drive letter in lowercase
    path = base::string_to_lower(path);
#endif

    m_breakpoints.erase(path);
    for (const auto& breakpoint : request.breakpoints.value()) {
      m_breakpoints.emplace(path, breakpoint.line);

      dap::Breakpoint bp;
      // TODO: Ideally we would store this Breakpoint object inside m_breakpoints and then verify
      // them after we run through them inside the hook, moving the line in case the breakpoint is
      // on an empty line.
      bp.verified = true;
      bp.line = breakpoint.line;
      response.breakpoints.push_back(bp);
    }

    return response;
  });

  m_session->registerHandler(
    [this](const dap::PauseRequest&) -> dap::ResponseOrError<dap::PauseResponse> {
      if (isWaiting())
        return dap::Error("Already paused");

      setHookState(State::Wait);
      stop("pause", 0);
      return dap::PauseResponse{};
    });

  m_session->registerHandler(
    [this](const dap::ContinueRequest&) -> dap::ResponseOrError<dap::ContinueResponse> {
      if (!isWaiting())
        return dap::Error("Execution is not paused");

      setHookState(State::Pass);
      return dap::ContinueResponse{};
    });

  m_session->registerHandler(
    [this](const dap::NextRequest&) -> dap::ResponseOrError<dap::NextResponse> {
      if (!isWaiting())
        return dap::Error("Execution is not paused");

      m_commandStackLevel = m_hookStackLevel;
      setHookState(State::NextLine);
      return dap::NextResponse{};
    });

  m_session->registerHandler(
    [this](const dap::StepInRequest&) -> dap::ResponseOrError<dap::StepInResponse> {
      if (!isWaiting())
        return dap::Error("Execution is not paused");

      m_commandStackLevel += 1;
      setHookState(State::NextLine);
      return dap::StepInResponse{};
    });

  m_session->registerHandler(
    [this](const dap::StepOutRequest&) -> dap::ResponseOrError<dap::StepOutResponse> {
      if (!isWaiting())
        return dap::Error("Execution is not paused");

      if (m_hookStackLevel == 0 || m_commandStackLevel == 0)
        return dap::Error("Already at the top level");

      m_commandStackLevel -= 1;
      setHookState(State::NextLine);
      return dap::StepOutResponse{};
    });

  m_session->registerHandler(
    [](const dap::DisconnectRequest&) { return dap::DisconnectResponse{}; });

  m_session->registerHandler(
    [this](const dap::StackTraceRequest&) -> dap::ResponseOrError<dap::StackTraceResponse> {
      const std::lock_guard lock(m_mutex);
      dap::StackTraceResponse response;
      response.stackFrames = m_currentStackTrace;
      return response;
    });

  m_session->registerHandler(
    [](const dap::ScopesRequest&) -> dap::ResponseOrError<dap::ScopesResponse> {
      dap::ScopesResponse response;
      dap::Scope locals;
      locals.expensive = false;
      locals.name = "Locals";
      locals.variablesReference = INT_MAX - 1;

      dap::Scope globals;
      globals.expensive = false;
      globals.name = "Globals";
      globals.variablesReference = INT_MAX;

      response.scopes = { locals, globals };
      return response;
    });

  m_session->registerHandler(
    [this](const dap::VariablesRequest& request) -> dap::ResponseOrError<dap::VariablesResponse> {
      dap::VariablesResponse response;
      if (request.variablesReference == INT_MAX - 1) {
        const std::lock_guard lock(m_mutex);
        response.variables = m_locals;
      }
      else if (request.variablesReference == INT_MAX) {
        const std::lock_guard lock(m_mutex);
        response.variables = m_globals;
      }
      else {
        std::promise<std::vector<dap::Variable>> promise;
        ui::execute_from_ui_thread([this, &request, &promise] {
          const auto& list = m_registry[request.variablesReference - 1];
          auto* L = m_extension ? m_extension->scriptEngine()->luaState() : m_engine->luaState();
          std::vector<dap::Variable> vars;
          vars.reserve(list.size());
          for (const auto& [name, index, ref] : list) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
            vars.push_back(makeVariable(L, name.empty() ? std::to_string(index) : name));
            lua_pop(L, 1);
          }
          promise.set_value(vars);
        });
        response.variables = promise.get_future().get();
      }

      return response;
    });

  m_session->registerHandler(
    [](const dap::ConfigurationDoneRequest&) { return dap::ConfigurationDoneResponse{}; });

  m_session->setOnInvalidData(dap::kClose);
  m_session->bind(rw, [this] {
    setHookState(State::Abort);

    ui::execute_from_ui_thread([this] {
      if (m_engine)
        m_engine->setDebugger(nullptr);
      if (m_extension) {
        if (m_extension->scriptEngine())
          m_extension->scriptEngine()->setDebugger(nullptr);
        m_extension = nullptr;
      }
    });
  });

  dap::ThreadEvent threadStartedEvent;
  threadStartedEvent.reason = "started";
  threadStartedEvent.threadId = DBG_THREAD_ID;
  m_session->send(threadStartedEvent);
}

} // namespace app::script
