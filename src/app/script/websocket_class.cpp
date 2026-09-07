// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/permissions.h"
#include "net/http_request.h"
#include "ui/manager.h"
#include "ui/message_loop.h"
#include "ui/system.h"

#include <algorithm>

#include <curl/curl.h>

namespace app::script {

namespace {

enum class WebSocketMessageType : uint8_t {
  Message = 0,
  Open = 1,
  Close = 2,
  Error = 3,
  Ping = 4,
  Pong = 5,
  Fragment = 6,
  // Value is 16 for backwards compatibility with the old flag
  Binary = 16
};

class WebSocketConnection {
public:
  struct ConnectionResult {
    bool success;
    CURLcode code;
    std::string errorString;
  };

  explicit WebSocketConnection(const std::string& url)
    : m_curl(curl_easy_init())
    , m_url(url)
    , m_outgoingOffset(0)
    , m_outgoingIsBinary(true)
    , m_reportedOpen(false)
    , m_shouldClose(false)
  {
    ASSERT(m_curl);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &WebSocketConnection::writeBodyCallback);
    curl_easy_setopt(m_curl, CURLOPT_READDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, &WebSocketConnection::readBodyCallback);
    curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1);
  }

  ~WebSocketConnection()
  {
    ASSERT(m_curl);
    curl_easy_cleanup(m_curl);
    m_curl = nullptr;
  }

  ConnectionResult connect() const
  {
    const auto result = curl_easy_perform(m_curl);
    if (result == CURLE_OK || result == CURLE_ABORTED_BY_CALLBACK)
      return { true, result, std::string() };

    return { false, result, curl_easy_strerror(result) };
  }

  void abort()
  {
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, 1);
    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, 1);

    m_shouldClose = true;
    curl_easy_pause(m_curl, CURLPAUSE_CONT);
  }

  obs::safe_signal<void(WebSocketMessageType, const std::vector<char>&)> Message;

  std::size_t receivedData(char* ptr, const std::size_t bytes)
  {
    const auto* frame = curl_ws_meta(m_curl);
    const std::vector buffer(ptr, ptr + bytes);
    if (frame->flags & CURLWS_CLOSE) {
      Message(WebSocketMessageType::Close, buffer);
    }
    else if (frame->flags & CURLWS_PING) {
      Message(WebSocketMessageType::Ping, buffer);
    }
    else if (frame->flags & CURLWS_PONG) {
      Message(WebSocketMessageType::Pong, buffer);
    }
    else if (frame->flags & CURLWS_BINARY) {
      Message(WebSocketMessageType::Binary, buffer);
    }
    else if (frame->flags & CURLWS_CONT) {
      Message(WebSocketMessageType::Fragment, buffer);
    }
    else {
      Message(WebSocketMessageType::Message, buffer);
    }
    return bytes;
  }

  static std::size_t writeBodyCallback(char* ptr,
                                       std::size_t size,
                                       std::size_t nitems,
                                       void* userdata)
  {
    auto* ws = static_cast<WebSocketConnection*>(userdata);
    if (ws->shouldClose())
      return 0;
    return ws->receivedData(ptr, size * nitems);
  }

  static std::size_t readBodyCallback(char* ptr, size_t size, size_t nitems, void* userdata)
  {
    auto* ws = static_cast<WebSocketConnection*>(userdata);
    return ws->readBody(ptr, size, nitems);
  }

  size_t readData(void* ptr, std::size_t size, std::size_t nitems)
  {
    const size_t room = size * nitems;
    const size_t remaining = m_outgoingData.size() - m_outgoingOffset;
    const size_t n = remaining < room ? remaining : room;
    if (m_outgoingOffset == 0 && !m_outgoingIsBinary) {
      // Curl default is binary, so we only need to flag this as text when it's not.
      curl_ws_start_frame(m_curl, CURLWS_TEXT, remaining);
    }
    if (n > 0) {
      std::memcpy(ptr, m_outgoingData.data() + m_outgoingOffset, n);
      m_outgoingOffset += n;
    }

    if (m_outgoingOffset == m_outgoingData.size()) {
      m_outgoingData.clear();
      m_outgoingOffset = 0;
    }

    return n;
  }

  std::size_t readBody(char* ptr, size_t size, size_t nitems)
  {
    if (shouldClose()) {
      Message(WebSocketMessageType::Close, std::vector<char>());
      return CURL_READFUNC_ABORT;
    }

    if (hasDataToSend())
      return readData(ptr, size, nitems);

    if (!m_reportedOpen) {
      Message(WebSocketMessageType::Open, std::vector<char>());
      m_reportedOpen = true;
    }

    return CURL_READFUNC_PAUSE;
  }

  bool shouldClose() const { return m_shouldClose; }

  bool hasDataToSend() const
  {
    if (m_outgoingData.empty())
      return false;

    return m_outgoingOffset < m_outgoingData.size();
  }

  void sendText(const std::string& text)
  {
    ASSERT(m_outgoingData.empty() && m_outgoingOffset == 0);
    m_outgoingData = std::vector<char>(text.begin(), text.end());
    m_outgoingOffset = 0;
    m_outgoingIsBinary = false;
    curl_easy_pause(m_curl, CURLPAUSE_CONT);
  }

  void sendData(std::vector<char>&& data)
  {
    ASSERT(m_outgoingData.empty() && m_outgoingOffset == 0);
    m_outgoingData = std::move(data);
    m_outgoingOffset = 0;
    m_outgoingIsBinary = true;
    curl_easy_pause(m_curl, CURLPAUSE_CONT);
  }

  void close()
  {
    m_shouldClose = true;
    curl_easy_pause(m_curl, CURLPAUSE_CONT);
  }

private:
  CURL* m_curl;
  std::string m_url;
  std::vector<char> m_outgoingData;

  size_t m_outgoingOffset;
  bool m_outgoingIsBinary;
  bool m_reportedOpen;
  std::atomic<bool> m_shouldClose;

  DISABLE_COPYING(WebSocketConnection);
};

class WebSocket {
public:
  WebSocket(lua_State* L,
            const std::string& url,
            int onrecieveRef,
            int minreconnectwait,
            int maxreconnectwait)
    : L(L)
    , m_url(url)
    , m_onreceiveRef(onrecieveRef)
    , m_minReconnectWait(200)
    , m_maxReconnectWait(1000)
    , m_connectionRequested(false)
  {
    if (minreconnectwait >= 0)
      m_minReconnectWait = minreconnectwait;
    if (maxreconnectwait > 0)
      m_maxReconnectWait = maxreconnectwait;
  }

  ~WebSocket()
  {
    if (m_conn) {
      m_messageConn.disconnect();
      m_conn->abort();
    }

    if (m_thread)
      m_thread->join();
  }

  void threadSocket()
  {
    m_conn = std::make_unique<WebSocketConnection>(m_url);
    m_messageConn = m_conn->Message.connect(
      [this](const WebSocketMessageType type, const std::vector<char>& buffer) {
        ui::execute_from_ui_thread([this, type, buffer] { onMessage(type, buffer); });
      });
    while (!m_connectionRequested && m_conn && !m_conn->shouldClose()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (!m_conn)
      return;

    size_t retryWait = m_minReconnectWait;
  retry:
    auto result = m_conn->connect();
    if (!m_conn->shouldClose()) {
      if (!result.success) {
        ui::execute_from_ui_thread([this, error = result.errorString] {
          onMessage(WebSocketMessageType::Error, std::vector(error.begin(), error.end()));
        });
      }

      if (retryWait < m_maxReconnectWait &&
          (result.code == CURLE_COULDNT_CONNECT || result.code == CURLE_OPERATION_TIMEDOUT ||
           result.code == CURLE_READ_ERROR || result.code == CURLE_RECV_ERROR)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(retryWait));
        // TODO: Use the actual increments from ixwebsocket instead of whatever this is
        retryWait = std::min<size_t>(retryWait + 200, m_maxReconnectWait);
        goto retry;
      }
    }
    m_connectionRequested = false;
    m_messageConn.disconnect();
    m_conn.reset();
  }

  void onMessage(WebSocketMessageType type, const std::vector<char>& buffer)
  {
    ui::assert_ui_thread();

    if (m_onreceiveRef < 0)
      return;

    lua_rawgeti(L, LUA_REGISTRYINDEX, m_onreceiveRef);
    lua_pushinteger(L, (int)type);
    if (type != WebSocketMessageType::Error) {
      lua_pushlstring(L, buffer.data(), buffer.size());
      lua_pushstring(L, "");
    }
    else {
      lua_pushstring(L, "");
      lua_pushlstring(L, buffer.data(), buffer.size());
    }

    if (lua_pcall(L, 3, 0, 0)) {
      if (const char* s = lua_tostring(L, -1)) {
        engine_print(L, s);
        if (m_conn) {
          const std::scoped_lock lock(m_mutex);
          m_conn->abort();
        }
      }
    }
  }

  void close()
  {
    if (!m_conn)
      return;
    const std::scoped_lock lock(m_mutex);
    m_conn->close();
  }

  void connect()
  {
    const std::scoped_lock lock(m_mutex);
    if (m_thread) {
      if (m_conn)
        m_conn->close();
      m_thread->join();
    }

    m_connectionRequested = true;

    if (!m_conn)
      m_thread = std::make_unique<std::thread>([this] { threadSocket(); });
  }

  bool isReady() const
  {
    return (m_conn != nullptr) && m_connectionRequested && !m_conn->shouldClose();
  }

  void waitForSent() const
  {
    ui::MessageLoop loop(ui::Manager::getDefault());
    while (m_conn != nullptr && m_conn->hasDataToSend())
      loop.pumpMessages();
  }

  void sendText(const std::string& text)
  {
    if (!m_conn)
      return;
    const std::scoped_lock lock(m_mutex);
    m_conn->sendText(text);
  }

  void sendBinary(std::vector<char>&& data)
  {
    if (!m_conn)
      return;
    const std::scoped_lock lock(m_mutex);
    m_conn->sendData(std::move(data));
  }

  const std::string& url() const { return m_url; }

private:
  lua_State* L;
  std::string m_url;
  int m_onreceiveRef;
  int m_minReconnectWait;
  int m_maxReconnectWait;

  obs::connection m_messageConn;
  std::atomic<bool> m_connectionRequested;
  std::mutex m_mutex;
  std::unique_ptr<WebSocketConnection> m_conn;
  std::unique_ptr<std::thread> m_thread;
};

int WebSocket_new(lua_State* L)
{
  std::string url;
  int onreceiveRef = -1;
  bool deflate = false;
  int minreconnectwait = -1;
  int maxreconnectwait = -1;

  if (lua_istable(L, 1)) {
    lua_getfield(L, 1, "url");
    if (const char* s = lua_tostring(L, -1)) {
      url = s;
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "deflate");
    if (lua_toboolean(L, -1)) {
      deflate = true;
    }
    lua_pop(L, 1);

    int type = lua_getfield(L, 1, "minreconnectwait");
    if (type == LUA_TNUMBER) {
      minreconnectwait = std::max<int>(0, 1000 * lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    type = lua_getfield(L, 1, "maxreconnectwait");
    if (type == LUA_TNUMBER) {
      maxreconnectwait = std::max<int>(0, 1000 * lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    type = lua_getfield(L, 1, "onreceive");
    if (type == LUA_TFUNCTION) {
      onreceiveRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else {
      lua_pop(L, 1);
    }
  }

  if (url.empty() || !net::is_valid_url(url))
    return luaL_error(L, "invalid url supplied to WebSocket");

  if (deflate)
    LOG(WARNING, "WebSocket does not currently support the 'deflate' option\n");

  get_engine(L)->accessGate(Permission::Network, url);
  push_new<WebSocket>(L, L, url, onreceiveRef, minreconnectwait, maxreconnectwait);
  get_engine(L)->trackObject();
  return 1;
}

int WebSocket_gc(lua_State* L)
{
  auto* ws = get_obj<WebSocket>(L, 1);
  ws->~WebSocket();
  get_engine(L)->untrackObject();
  return 0;
}

std::string get_send_data(lua_State* L, WebSocket* ws)
{
  if (!ws->isReady())
    luaL_error(L, "WebSocket is not connected, can't send text");

  std::stringstream data;
  int argc = lua_gettop(L);
  for (int i = 2; i <= argc; i++) {
    size_t bufLen;
    const char* buf = lua_tolstring(L, i, &bufLen);
    data.write(buf, bufLen);
  }

  return data.str();
}

int WebSocket_sendText(lua_State* L)
{
  auto* ws = get_obj<WebSocket>(L, 1);
  const auto& data = get_send_data(L, ws);
  ws->waitForSent();
  ws->sendText(data);
  return 0;
}

int WebSocket_sendBinary(lua_State* L)
{
  auto* ws = get_obj<WebSocket>(L, 1);
  const auto& str = get_send_data(L, ws);
  auto data = std::vector<char>(str.begin(), str.end());
  ws->waitForSent();
  ws->sendBinary(std::move(data));
  return 0;
}

int WebSocket_sendPing(lua_State* L)
{
  // TODO: Curl autopings so we should probably deprecate this and keep it no-op, maybe a devmode
  // warning.
  return 0;
}

int WebSocket_connect(lua_State* L)
{
  auto* ws = get_obj<WebSocket>(L, 1);
  ws->connect();
  return 0;
}

int WebSocket_close(lua_State* L)
{
  auto* ws = get_obj<WebSocket>(L, 1);
  ws->close();
  return 0;
}

int WebSocket_get_url(lua_State* L)
{
  const auto* ws = get_obj<WebSocket>(L, 1);
  lua_pushstring(L, ws->url().c_str());
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
DEF_ITERATOR_PAIRS(WebSocket);
} // anonymous namespace

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
  setfield_integer(L, "TEXT", WebSocketMessageType::Message);
  setfield_integer(L, "BINARY", WebSocketMessageType::Binary);
  setfield_integer(L, "OPEN", WebSocketMessageType::Open);
  setfield_integer(L, "CLOSE", WebSocketMessageType::Close);
  setfield_integer(L, "ERROR", WebSocketMessageType::Error);
  setfield_integer(L, "PING", WebSocketMessageType::Ping);
  setfield_integer(L, "PONG", WebSocketMessageType::Pong);
  setfield_integer(L, "FRAGMENT", WebSocketMessageType::Fragment);
  lua_pop(L, 1);
}

} // namespace app::script
