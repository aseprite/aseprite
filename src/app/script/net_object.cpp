// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/script/luacpp.h"

#ifdef ENABLE_SCRIPT_NET
  #include "app/console.h"
  #include "app/i18n/strings.h"
  #include "app/script/engine.h"
  #include "app/task.h"
  #include "base/base64.h"
  #include "base/string.h"
  #include "net/http_headers.h"
  #include "net/http_request.h"
  #include "net/http_response.h"
  #include "ui/system.h"
  #include "ver/info.h"

  #include "fmt/format.h"
#endif

namespace app::script {
#ifdef ENABLE_SCRIPT_NET
namespace {
struct AppNet {};

constexpr int kDefaultTimeout = 30000;

int AppNet_urlEncode(lua_State* L)
{
  const char* input = luaL_checkstring(L, 1);
  lua_pushstring(L, net::url_encode(input).c_str());
  return 1;
}

int AppNet_urlDecode(lua_State* L)
{
  const char* input = luaL_checkstring(L, 1);
  lua_pushstring(L, net::url_decode(input).c_str());
  return 1;
}

int AppNet_fromBase64(lua_State* L)
{
  const char* input = luaL_checkstring(L, 1);
  auto out = base::decode_base64s(input);
  lua_pushlstring(L, out.c_str(), out.size());
  return 1;
}

int AppNet_toBase64(lua_State* L)
{
  size_t len;
  const auto* bytes = lua_tolstring(L, 1, &len);
  lua_pushstring(L, base::encode_base64(std::string(bytes, len)).c_str());
  return 1;
}

class FetchRequest {
public:
  struct Params {
    std::string url;
    net::HttpRequest::Method method = net::HttpRequest::Method::GET;
    std::string body;
    int onreceiveRef = -1;
    net::HttpHeaders headers;

    bool hasError() const { return onreceiveRef < 0 || url.empty() || !net::is_valid_url(url); }
  };

  struct Response {
    std::string body;
    int status = 0;
    net::HttpHeaders headers;
  };

  FetchRequest(lua_State* L, Params& params)
    : L(L)
    , m_params(std::move(params))
    , m_thread([this] { threadRun(); })
  {
    get_engine(L)->trackObject();
  }

  ~FetchRequest()
  {
    if (m_thread.joinable())
      m_thread.join();
    get_engine(L)->untrackObject();
  }

  void threadRun()
  {
    std::stringstream output;
    // TODO: We don't have a way right now to abort the request because HttpResponse itself doesn't
    // provide one, we need a refactor of that.
    const auto request = std::make_unique<net::HttpRequest>(m_params.url, m_params.method);
    if (!m_params.body.empty())
      request->setPostFields(m_params.body);
    request->setHeaders(m_params.headers);
    net::HttpResponse response(&output);
    if (request->send(response, kDefaultTimeout)) {
      m_response.body = output.str();
      m_response.headers = response.headers();
      m_response.status = response.status();
    }
    else {
      m_response.body = response.error();
    }

    Done();
  }

  void callOnReceived()
  {
    lua_rawgeti(L, LUA_REGISTRYINDEX, m_params.onreceiveRef);

    lua_newtable(L);
    lua_pushstring(L, m_response.body.c_str());
    lua_setfield(L, -2, "text");

    lua_pushlstring(L, m_response.body.c_str(), m_response.body.size());
    lua_setfield(L, -2, "blob");

    lua_newtable(L);
    for (const auto& [name, value] : m_response.headers) {
      lua_pushstring(L, value.c_str());
      lua_setfield(L, -2, name.c_str());
    }
    lua_setfield(L, -2, "headers");

    const auto& contentType = m_response.headers.getHeader("Content-Type");
    if (contentType.find("application/json") == 0) {
      lua_getglobal(L, "json");
      lua_getfield(L, -1, "decode");
      lua_remove(L, -2);
      lua_pushstring(L, m_response.body.c_str());
      if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
        // Eat the error and fill it with null if it's invalid json
        lua_pop(L, 1);
        lua_pushnil(L);
      }
    }
    else {
      lua_pushnil(L);
    }
    lua_setfield(L, -2, "json");
    setfield_integer(L, "status", m_response.status);

    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
      if (const char* s = lua_tostring(L, -1))
        Console::println(s);
    }
  }

  obs::safe_signal<void()> Done;

private:
  lua_State* L;
  Params m_params;
  Response m_response;
  std::thread m_thread;
};

std::set<FetchRequest*> g_requests;

int AppNet_fetch(lua_State* L)
{
  FetchRequest::Params params;
  std::string methodString = "get";

  params.headers.setHeader(
    "User-Agent",
    fmt::format("Mozilla/5.0 (compatible; {}/{}; +https://www.aseprite.org/api/app#fetch)",
                get_app_name(),
                get_app_version()));
  params.headers.setHeader("Accept", "application/json, text/plain, */*");
  params.headers.setHeader("Accept-Language", Strings::instance()->currentLanguage());

  if (lua_istable(L, 1)) {
    lua_getfield(L, 1, "url");
    if (const char* s = lua_tostring(L, -1))
      params.url = s;
    lua_pop(L, 1);

    lua_getfield(L, 1, "method");
    if (const char* s = lua_tostring(L, -1))
      methodString = base::string_to_lower(s);
    lua_pop(L, 1);

    int type = lua_getfield(L, 1, "onreceive");
    if (type == LUA_TFUNCTION) {
      params.onreceiveRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else {
      lua_pop(L, 1);
    }

    type = lua_getfield(L, 1, "body");
    if (type == LUA_TSTRING || type == LUA_TNUMBER) {
      params.headers.setHeader("Content-Type", "text/plain");
      params.body = lua_tostring(L, -1);
    }
    else if (type == LUA_TTABLE) {
      params.headers.setHeader("Content-Type", "application/x-www-form-urlencoded");
      lua_pushnil(L);
      while (lua_next(L, -2) != 0) {
        if (!params.body.empty())
          params.body += '&';

        lua_pushvalue(L, -2);
        const char* key = lua_tostring(L, -1);
        const char* value = lua_tostring(L, -2);
        if (key != nullptr) {
          params.body += net::url_encode(key);
          params.body += '=';
          if (value) // Supports sending empty values
            params.body += net::url_encode(value);
        }
        else {
          lua_pop(L, 2);
          return luaL_error(L, "invalid 'body' argument");
        }
        lua_pop(L, 2);
      }
    }
    else if (type == LUA_TUSERDATA) {
      if (luaL_getmetafield(L, -1, "__typename") == LUA_TSTRING) {
        if (const auto* typeName = lua_tostring(L, -1); std::strcmp(typeName, "JsonObj") != 0) {
          return luaL_error(L, "'body' arguments must be a string, table or json object");
        }
        lua_pop(L, 1);
      }
      else {
        return luaL_error(L, "'body' arguments must be a string, table or json object");
      }
      size_t len;
      const auto* s = luaL_tolstring(L, -1, &len);
      params.body = std::string(s, len);
      params.headers.setHeader("Content-Type", "application/json");
      lua_pop(L, 1);
    }
    lua_pop(L, 1);

    type = lua_getfield(L, 1, "headers");
    if (type == LUA_TTABLE) {
      lua_pushnil(L);
      while (lua_next(L, -2) != 0) {
        lua_pushvalue(L, -2);
        const char* key = lua_tostring(L, -1);
        const char* value = lua_tostring(L, -2);
        if (key != nullptr && value != nullptr) {
          params.headers.setHeader(key, value);
        }
        lua_pop(L, 2);
      }
    }
    lua_pop(L, 1);
  }

  if (methodString == "get")
    params.method = net::HttpRequest::Method::GET;
  else if (methodString == "post")
    params.method = net::HttpRequest::Method::POST;
  else if (methodString == "put")
    params.method = net::HttpRequest::Method::PUT;
  else if (methodString == "patch")
    params.method = net::HttpRequest::Method::PATCH;
  else if (methodString == "options")
    params.method = net::HttpRequest::Method::OPTIONS;
  else
    return luaL_error(L, "invalid 'method', must be one of GET, POST, PUT, PATCH or OPTIONS");

  if (params.hasError())
    return luaL_error(L, "invalid arguments supplied to 'fetch'");

  get_engine(L)->accessGate(Permission::Network, params.url);

  auto* request = new FetchRequest(L, params);
  g_requests.emplace(request);
  request->Done.connect([request] {
    ui::execute_from_ui_thread([request] {
      request->callOnReceived();
      g_requests.erase(request);
      delete request;
    });
  });

  return 0;
}

const luaL_Reg AppNet_methods[] = {
  { "urlEncode",  AppNet_urlEncode  },
  { "urlDecode",  AppNet_urlDecode  },
  { "fromBase64", AppNet_fromBase64 },
  { "toBase64",   AppNet_toBase64   },
  { "fetch",      AppNet_fetch      },
  { nullptr,      nullptr           }
};

} // namespace

DEF_MTNAME(AppNet);

void register_app_net_object(lua_State* L)
{
  REG_CLASS(L, AppNet);
  lua_getglobal(L, "app");
  lua_pushstring(L, "net");
  push_new<AppNet>(L);
  lua_rawset(L, -3);
  lua_pop(L, 1);
}
#else
void register_app_net_object(lua_State* L)
{
  lua_getglobal(L, "app");
  lua_pushstring(L, "net");
  lua_pushnil(L);
  lua_rawset(L, -3);
  lua_pop(L, 1);
}
#endif
} // namespace app::script
