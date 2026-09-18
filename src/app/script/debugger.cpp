// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/i18n/strings.h"
#include "app/resource_finder.h"
#include "app/script/debugger.h"
#include "app/script/engine_manager.h"
#include "app/script/luacpp.h"
#include "base/exception.h"
#include "base/fs.h"
#include "doc/layer.h"
#include "ui/manager.h"
#include "ui/message_loop.h"
#include "ui/register_message.h"
#include "ui/system.h"

#include "dap/io.h"
#include "dap/network.h"
#include "dap/protocol.h"
#include "dap/session.h"

#include "fmt/format.h"

#include <future>

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
constexpr int kLocals = std::numeric_limits<int>::max() - 1;
constexpr int kGlobals = std::numeric_limits<int>::max();

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
  ui::assert_ui_thread();

  // cppdap hangs inside a mutex lock if we attempt to start a server when there's already one open
  // so we need to test it beforehand.
  if (dap::net::connect(address.c_str(), port, 100))
    throw base::Exception(Strings::debugger_error_in_use());

  m_server = dap::net::Server::create();
  const bool result = m_server->start(
    address.c_str(),
    port,
    [this](const auto& rw) { onClientConnected(rw); },
    [this](const char* msg) {
      Error(Strings::debugger_error(msg));
      m_session.reset();
    });
  if (!result)
    throw base::Exception(Strings::debugger_error_server_creation());

  m_threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
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
      frame.name = Strings::debugger_main();
    }
    else {
      frame.name = Strings::debugger_unnamed_function();
    }

    if (ar.istailcall)
      frame.name += " " + Strings::debugger_tail_call();
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
      if (auto* str = lua_tostring(L, -1)) {
        // TODO: Having a non-utf8 character in this string (such as raw bytes from an image or a
        // network download) can crash nlohmann::json so we're gonna have to detect that and trim
        // them or something like that
        var.value = str;
      }
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
void Debugger::stop(const std::string& reason, const dap::integer)
{
  dap::StoppedEvent event;
  event.reason = reason;
  event.threadId = m_threadId;
  m_session->send(event);
  setHookState(State::Wait);
}

void Debugger::onHook(lua_State* L, lua_Debug* ar)
{
  ASSERT(isDebugging());
  ASSERT(m_session);
  ASSERT(ar);

  m_lastHookTick = base::current_tick();

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
  if (isDebugging()) {
    // Close any incoming connection attempts while we're already debugging.
    Error(Strings::debugger_error_in_use());
    rw->close();
    return;
  }

  // Clear everything before we allow another client.
  m_currentStackTrace.clear();
  m_breakpoints.clear();
  m_locals.clear();
  m_globals.clear();
  m_commandStackLevel = 0;
  m_hookStackLevel = 0;

  m_session = dap::Session::create();

  registerHandlers();

  m_session->setOnInvalidData(dap::kClose);
  m_session->bind(rw, [this] { onSessionClosed(); });

  dap::ThreadEvent threadStartedEvent;
  threadStartedEvent.reason = "started";
  threadStartedEvent.threadId = m_threadId;
  m_session->send(threadStartedEvent);
}

void Debugger::registerHandlers()
{
  m_session->onError([this](const char* msg) { onSessionError(msg); });

  // Static response handlers
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
  m_session->registerHandler(
    [](const dap::ConfigurationDoneRequest&) { return dap::ConfigurationDoneResponse{}; });

  // Main handlers
  m_session->registerHandler(
    [this](const dap::ThreadsRequest& request) { return onThreads(request); });
  m_session->registerHandler([this](const AseAttachRequest& request) { return onAttach(request); });
  m_session->registerHandler([this](const AseLaunchRequest& request) { return onLaunch(request); });
  m_session->registerHandler(
    [this](const dap::SetBreakpointsRequest& request) { return onSetBreakpoints(request); });
  m_session->registerHandler([this](const dap::PauseRequest& request) { return onPause(request); });
  m_session->registerHandler(
    [this](const dap::ContinueRequest& request) { return onContinue(request); });
  m_session->registerHandler([this](const dap::NextRequest& request) { return onNext(request); });
  m_session->registerHandler(
    [this](const dap::StepInRequest& request) { return onStepIn(request); });
  m_session->registerHandler(
    [this](const dap::StepOutRequest& request) { return onStepOut(request); });
  m_session->registerHandler(
    [](const dap::DisconnectRequest&) { return dap::DisconnectResponse{}; });
  m_session->registerHandler(
    [this](const dap::StackTraceRequest& request) { return onStackTrace(request); });
  m_session->registerHandler(
    [this](const dap::ScopesRequest& request) { return onScopes(request); });
  m_session->registerHandler(
    [this](const dap::VariablesRequest& request) { return onVariables(request); });
}

void Debugger::onSessionError(const char* msg)
{
  Error(fmt::format("Debugger session error: {}", msg));
}

dap::ThreadsResponse Debugger::onThreads(const dap::ThreadsRequest&) const
{
  dap::ThreadsResponse response;
  dap::Thread thread;
  thread.id = m_threadId;
  thread.name = "Main";
  response.threads.push_back(thread);
  return response;
}

dap::ResponseOrError<dap::AttachResponse> Debugger::onAttach(const AseAttachRequest& request)
{
  if (isDebugging())
    return dap::Error(Strings::debugger_error_no_simultaneous_debugger());

  if (!request.extensionName.has_value() || request.extensionName.value().empty())
    return dap::Error(Strings::debugger_error_requires_argument("extensionName"));

  const auto& name = request.extensionName.value();
  for (Extension* ext : App::instance()->extensions()) {
    if (ext->name() == name) {
      if (!ext->isEnabled())
        return dap::Error(Strings::debugger_error_extension_not_enabled());

      if (!ext->hasScripts())
        return dap::Error(Strings::debugger_error_extension_no_scripts());

      m_extension = ext;
      setHookState(State::Pass);
      ui::execute_from_ui_thread([this] { m_extension->scriptEngine()->setDebugger(this); });

      return dap::AttachResponse{};
    }
  }

  return dap::Error(Strings::debugger_error_extension_not_found(name));
}

dap::ResponseOrError<dap::LaunchResponse> Debugger::onLaunch(const AseLaunchRequest& request)
{
  if (request.noDebug)
    return dap::Error(Strings::debugger_error_unsupported());

  if (isDebugging())
    return dap::Error(Strings::debugger_error_no_simultaneous_debugger());

  if ((!request.extensionName.has_value() || request.extensionName.value().empty()) &&
      (!request.scriptFilename.has_value() || request.scriptFilename.value().empty()))
    return dap::Error(
      Strings::debugger_error_requires_arguments("extensionName", "scriptFilename"));

  if (request.scriptFilename.has_value() && !request.scriptFilename.value().empty()) {
    std::string filename = request.scriptFilename.value();

    if (!base::is_absolute_path(filename) && !base::is_file(filename)) {
      ResourceFinder rf;
      rf.includeUserDir(base::join_path("scripts", filename));
      if (rf.findFirst())
        filename = rf.filename();
    }
    filename = base::normalize_path(filename);
    if (!base::is_file(filename))
      return dap::Error(Strings::debugger_error_script_not_found());

    ui::execute_from_ui_thread([this, filename] {
      setHookState(State::Pass);
      m_engine = EngineManager::create();
      std::string lastError;
      m_engine->ConsoleError.connect([&lastError](const std::string& error) { lastError = error; });
      m_engine->setDebugger(this);
      m_engine->evalUserFile(filename);

      if (m_engine->returnCode() < 0) {
        dap::StoppedEvent event;
        event.description = Strings::debugger_error_script_error();
        event.reason = "exception";
        event.text = lastError;
        m_session->send(event);
      }

      if (m_engine->hasLingeringObjects())
        return;

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
          return dap::Error(Strings::debugger_error_extension_no_scripts());

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
      return dap::Error(Strings::debugger_error_extension_not_found(request.extensionName.value()));
  }

  return dap::LaunchResponse{};
}

dap::ResponseOrError<dap::SetBreakpointsResponse> Debugger::onSetBreakpoints(
  const dap::SetBreakpointsRequest& request)
{
  const std::lock_guard lock(m_mutex);

  if (!request.breakpoints.has_value())
    return dap::SetBreakpointsResponse{};

  if (!request.source.path.has_value())
    return dap::Error(Strings::debugger_error_bp_source_not_found());

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
}

dap::ResponseOrError<dap::PauseResponse> Debugger::onPause(const dap::PauseRequest&)
{
  if (isWaiting())
    return dap::Error(Strings::debugger_error_already_paused());

  setHookState(State::Wait);
  stop("pause", 0);
  return dap::PauseResponse{};
}

dap::ResponseOrError<dap::ContinueResponse> Debugger::onContinue(const dap::ContinueRequest&)
{
  if (!isWaiting())
    return dap::Error(Strings::debugger_error_is_not_paused());

  setHookState(State::Pass);
  return dap::ContinueResponse{};
}

dap::ResponseOrError<dap::NextResponse> Debugger::onNext(const dap::NextRequest&)
{
  if (!isWaiting())
    return dap::Error(Strings::debugger_error_is_not_paused());

  m_commandStackLevel = m_hookStackLevel;
  setHookState(State::NextLine);
  return dap::NextResponse{};
}

dap::ResponseOrError<dap::StepInResponse> Debugger::onStepIn(const dap::StepInRequest&)
{
  if (!isWaiting())
    return dap::Error(Strings::debugger_error_is_not_paused());

  m_commandStackLevel += 1;
  setHookState(State::NextLine);
  return dap::StepInResponse{};
}

dap::ResponseOrError<dap::StepOutResponse> Debugger::onStepOut(const dap::StepOutRequest&)
{
  if (!isWaiting())
    return dap::Error(Strings::debugger_error_is_not_paused());

  if (m_hookStackLevel == 0 || m_commandStackLevel == 0)
    return dap::Error(Strings::debugger_error_already_top_level());

  m_commandStackLevel -= 1;
  setHookState(State::NextLine);
  return dap::StepOutResponse{};
}

dap::ResponseOrError<dap::StackTraceResponse> Debugger::onStackTrace(const dap::StackTraceRequest&)
{
  const std::lock_guard lock(m_mutex);
  dap::StackTraceResponse response;
  response.stackFrames = m_currentStackTrace;
  return response;
}

dap::ResponseOrError<dap::ScopesResponse> Debugger::onScopes(const dap::ScopesRequest&)
{
  dap::ScopesResponse response;
  dap::Scope locals;
  locals.expensive = false;
  locals.name = Strings::debugger_locals();
  locals.variablesReference = kLocals;

  dap::Scope globals;
  globals.expensive = false;
  globals.name = Strings::debugger_globals();
  globals.variablesReference = kGlobals;

  response.scopes = { locals, globals };
  return response;
}

dap::ResponseOrError<dap::VariablesResponse> Debugger::onVariables(
  const dap::VariablesRequest& request)
{
  dap::VariablesResponse response;
  if (request.variablesReference == kLocals) {
    const std::lock_guard lock(m_mutex);
    response.variables = m_locals;
  }
  else if (request.variablesReference == kGlobals) {
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
}

void Debugger::onSessionClosed()
{
  if (!isDebugging())
    return;

  // Alternate path to handle when the debugger disconnects:
  const bool wasBreaking = m_hookState.load() != State::Pass && m_hookState.load() != State::Abort;
  setHookState(State::Abort);

  if (wasBreaking) {
    // We don't have a way to know when the currently stopped script finished execution so we're
    // going to try to sleep periodically and check that enough time has passed between the last
    // time our hook was called, which is something that should happen every instruction and thus
    // keep incrementing while we're still using the Lua state.
    do {
      std::this_thread::sleep_for(std::chrono::milliseconds(150));
      if (base::current_tick() - m_lastHookTick > 200)
        break;
    } while (true);
  }

  ui::execute_from_ui_thread([this] {
    if (m_engine) {
      m_engine->setDebugger(nullptr);
      m_engine.reset();
    }
    if (m_extension) {
      if (m_extension->scriptEngine())
        m_extension->scriptEngine()->setDebugger(nullptr);
      m_extension = nullptr;
    }
  });
}

} // namespace app::script
