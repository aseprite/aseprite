// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "ui/timer.h"

#include <algorithm>

namespace app { namespace script {

namespace {

class Timer : public ui::Timer {
public:
  Timer() : ui::Timer(100) {}

  int runningRef() const { return m_runningRef; }

  void refTimer(lua_State* L)
  {
    if (m_runningRef == LUA_REFNIL) {
      lua_pushvalue(L, 1);
      m_runningRef = luaL_ref(L, LUA_REGISTRYINDEX);
    }
  }

  void unrefTimer(lua_State* L)
  {
    if (m_runningRef != LUA_REFNIL) {
      luaL_unref(L, LUA_REGISTRYINDEX, m_runningRef);
      m_runningRef = LUA_REFNIL;
    }
  }

private:
  // Reference used to keep the timer alive (so it's not garbage
  // collected) when it's running.
  int m_runningRef = LUA_REFNIL;
};

int Timer_new(lua_State* L)
{
  auto timer = push_new<Timer>(L);

  if (lua_istable(L, 1)) {
    auto type = lua_getfield(L, 1, "interval");
    if (type != LUA_TNIL)
      timer->setInterval(std::max(1, int(lua_tonumber(L, -1) * 1000.0)));
    lua_pop(L, 1);

    type = lua_getfield(L, 1, "ontick");
    if (type == LUA_TFUNCTION) {
      lua_pushvalue(L, -1);    // Copy the function in the stack
      lua_setuservalue(L, -3); // Put the function as uservalue of the timer

      timer->Tick.connect([timer, L]() {
        if (timer->runningRef() == LUA_REFNIL)
          return;

        try {
          // Get the timer, and get the function that is inside the
          // timer uservalue
          lua_rawgeti(L, LUA_REGISTRYINDEX, timer->runningRef());
          lua_getuservalue(L, -1);
          if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0)) {
              if (const char* s = lua_tostring(L, -1))
                App::instance()->scriptEngine()->consolePrint(s);
            }
          }
          else {
            lua_pop(L, 1); // Pop the value which should have been a function
          }
          lua_pop(L, 1); // Pop timer
        }
        catch (const std::exception& ex) {
          App::instance()->scriptEngine()->consolePrint(ex.what());
        }
      });
    }
    lua_pop(L, 1);
  }
  return 1;
}

int Timer_gc(lua_State* L)
{
  auto obj = get_obj<Timer>(L, 1);
  obj->~Timer();
  return 0;
}

int Timer_start(lua_State* L)
{
  auto obj = get_obj<Timer>(L, 1);
  obj->start();
  obj->refTimer(L);
  return 0;
}

int Timer_stop(lua_State* L)
{
  auto obj = get_obj<Timer>(L, 1);
  obj->stop();
  obj->unrefTimer(L);
  return 0;
}

int Timer_get_interval(lua_State* L)
{
  const auto timer = get_obj<Timer>(L, 1);
  lua_pushnumber(L, double(timer->interval()) / 1000.0);
  return 1;
}

int Timer_set_interval(lua_State* L)
{
  const auto timer = get_obj<Timer>(L, 1);
  int interval = int(lua_tonumber(L, 2) * 1000.0);
  timer->setInterval(std::max(1, interval));
  return 1;
}

int Timer_get_isRunning(lua_State* L)
{
  const auto timer = get_obj<Timer>(L, 1);
  lua_pushboolean(L, timer->isRunning());
  return 1;
}

const luaL_Reg Timer_methods[] = {
  { "__gc",  Timer_gc    },
  { "start", Timer_start },
  { "stop",  Timer_stop  },
  { nullptr, nullptr     }
};

const Property Timer_properties[] = {
  { "interval",  Timer_get_interval,  Timer_set_interval },
  { "isRunning", Timer_get_isRunning, nullptr            },
  { nullptr,     nullptr,             nullptr            }
};

} // anonymous namespace

DEF_MTNAME(Timer);

void register_timer_class(lua_State* L)
{
  REG_CLASS(L, Timer);
  REG_CLASS_NEW(L, Timer);
  REG_CLASS_PROPERTIES(L, Timer);
}

}} // namespace app::script
