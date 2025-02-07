// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/console.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/security.h"
#include "ui/manager.h"
#include "ui/system.h"
#include "ui/timer.h"

#include <ixwebsocket/IXNetSystem.h>
#include <ixwebsocket/IXWebSocket.h>
#include <set>
#include <sstream>

namespace app { namespace script {

namespace {

// Additional "enum" value to make message callback simpler
#define MESSAGE_TYPE_BINARY ((int)ix::WebSocketMessageType::Fragment + 10)

static std::unique_ptr<ui::Timer> g_timer;
static std::set<ix::WebSocket*> g_connections;

static void close_ws(ix::WebSocket* ws)
{
  ws->stop();

  g_connections.erase(ws);
  if (g_connections.empty())
    g_timer.reset();
}

int WebSocket_new(lua_State* L)
{
  static std::once_flag f;
  std::call_once(f, &ix::initNetSystem);

  auto ws = new ix::WebSocket();

  push_ptr(L, ws);

  if (lua_istable(L, 1)) {
    lua_getfield(L, 1, "url");
    if (const char* s = lua_tostring(L, -1)) {
      if (!ask_access(L, s, FileAccessMode::OpenSocket, ResourceType::WebSocket))
        return luaL_error(L, "the script doesn't have access to create a WebSocket for '%s'", s);

      ws->setUrl(s);
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "deflate");
    if (lua_toboolean(L, -1)) {
      ws->enablePerMessageDeflate();
    }
    else {
      ws->disablePerMessageDeflate();
    }
    lua_pop(L, 1);

    int type = lua_getfield(L, 1, "minreconnectwait");
    if (type == LUA_TNUMBER) {
      ws->setMinWaitBetweenReconnectionRetries(1000 * lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    type = lua_getfield(L, 1, "maxreconnectwait");
    if (type == LUA_TNUMBER) {
      ws->setMaxWaitBetweenReconnectionRetries(1000 * lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    type = lua_getfield(L, 1, "onreceive");
    if (type == LUA_TFUNCTION) {
      int onreceiveRef = luaL_ref(L, LUA_REGISTRYINDEX);

      ws->setOnMessageCallback([L, ws, onreceiveRef](const ix::WebSocketMessagePtr& msg) {
        int msgType = (msg->binary ? MESSAGE_TYPE_BINARY : static_cast<int>(msg->type));
        std::string msgData = msg->str;

        ui::execute_from_ui_thread([=]() {
          lua_rawgeti(L, LUA_REGISTRYINDEX, onreceiveRef);
          lua_pushinteger(L, msgType);
          lua_pushlstring(L, msgData.c_str(), msgData.length());

          if (lua_pcall(L, 2, 0, 0)) {
            if (const char* s = lua_tostring(L, -1)) {
              App::instance()->scriptEngine()->consolePrint(s);
              ws->stop();
            }
          }
        });
      });
    }
    else {
      // Set a default handler to avoid a std::bad_function_call exception
      ws->setOnMessageCallback([](const ix::WebSocketMessagePtr& msg) {});
      lua_pop(L, 1);
    }
  }

  return 1;
}

int WebSocket_gc(lua_State* L)
{
  auto ws = get_ptr<ix::WebSocket>(L, 1);
  close_ws(ws);
  delete ws;
  return 0;
}

int WebSocket_sendText(lua_State* L)
{
  auto ws = get_ptr<ix::WebSocket>(L, 1);

  if (ws->getReadyState() != ix::ReadyState::Open) {
    return luaL_error(L, "WebSocket is not connected, can't send text");
  }

  std::stringstream data;
  int argc = lua_gettop(L);

  for (int i = 2; i <= argc; i++) {
    size_t bufLen;
    const char* buf = lua_tolstring(L, i, &bufLen);
    data.write(buf, bufLen);
  }

  if (!ws->sendText(data.str()).success) {
    return luaL_error(L, "WebSocket failed to send text");
  }
  return 0;
}

int WebSocket_sendBinary(lua_State* L)
{
  auto ws = get_ptr<ix::WebSocket>(L, 1);

  if (ws->getReadyState() != ix::ReadyState::Open) {
    return luaL_error(L, "WebSocket is not connected, can't send data");
  }

  std::stringstream data;
  int argc = lua_gettop(L);

  for (int i = 2; i <= argc; i++) {
    size_t bufLen;
    const char* buf = lua_tolstring(L, i, &bufLen);
    data.write(buf, bufLen);
  }

  if (!ws->sendBinary(data.str()).success) {
    return luaL_error(L, "WebSocket failed to send data");
  }
  return 0;
}

int WebSocket_sendPing(lua_State* L)
{
  auto ws = get_ptr<ix::WebSocket>(L, 1);

  if (ws->getReadyState() != ix::ReadyState::Open) {
    return luaL_error(L, "WebSocket is not connected, can't send ping");
  }

  size_t bufLen;
  const char* buf = lua_tolstring(L, 2, &bufLen);
  std::string data(buf, bufLen);

  if (!ws->ping(data).success) {
    return luaL_error(L, "WebSocket failed to send ping");
  }
  return 0;
}

int WebSocket_connect(lua_State* L)
{
  auto ws = get_ptr<ix::WebSocket>(L, 1);
  ws->start();

  if (g_connections.empty()) {
    if (App::instance()->isGui()) {
      g_timer = std::make_unique<ui::Timer>(33, ui::Manager::getDefault());
      g_timer->start();
    }
  }
  g_connections.insert(ws);

  return 0;
}

int WebSocket_close(lua_State* L)
{
  auto ws = get_ptr<ix::WebSocket>(L, 1);
  close_ws(ws);
  return 0;
}

int WebSocket_get_url(lua_State* L)
{
  auto ws = get_ptr<ix::WebSocket>(L, 1);
  lua_pushstring(L, ws->getUrl().c_str());
  return 1;
}

const luaL_Reg WebSocket_methods[] = {
  { "__gc",       WebSocket_gc         },
  { "close",      WebSocket_close      },
  { "connect",    WebSocket_connect    },
  { "sendText",   WebSocket_sendText   },
  { "sendBinary", WebSocket_sendBinary },
  { "sendPing",   WebSocket_sendPing   },
  { nullptr,      nullptr              }
};

const Property WebSocket_properties[] = {
  { "url",   WebSocket_get_url, nullptr },
  { nullptr, nullptr,           nullptr }
};

} // anonymous namespace

using WebSocket = ix::WebSocket;
DEF_MTNAME(WebSocket);

void register_websocket_class(lua_State* L)
{
  REG_CLASS(L, WebSocket);
  REG_CLASS_NEW(L, WebSocket);
  REG_CLASS_PROPERTIES(L, WebSocket);

  // message type enum
  lua_newtable(L);
  lua_pushvalue(L, -1);
  lua_setglobal(L, "WebSocketMessageType");
  setfield_integer(L, "TEXT", (int)ix::WebSocketMessageType::Message);
  setfield_integer(L, "BINARY", MESSAGE_TYPE_BINARY);
  setfield_integer(L, "OPEN", (int)ix::WebSocketMessageType::Open);
  setfield_integer(L, "CLOSE", (int)ix::WebSocketMessageType::Close);
  setfield_integer(L, "ERROR", (int)ix::WebSocketMessageType::Error);
  setfield_integer(L, "PING", (int)ix::WebSocketMessageType::Ping);
  setfield_integer(L, "PONG", (int)ix::WebSocketMessageType::Pong);
  setfield_integer(L, "FRAGMENT", (int)ix::WebSocketMessageType::Fragment);
  lua_pop(L, 1);
}

}} // namespace app::script
