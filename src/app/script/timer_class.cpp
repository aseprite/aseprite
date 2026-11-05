// Aseprite
// Copyright (C) 2022-present  Igara Studio S.A.
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
  explicit Timer(lua_State* L) : ui::Timer(100), L(L) {}
  ~Timer() override { unrefTimer(); }

  int runningRef() const { return m_runningRef; }

  void refTimer()
  {
    if (m_runningRef == LUA_REFNIL) {
      lua_pushvalue(L, 1);
      m_runningRef = luaL_ref(L, LUA_REGISTRYINDEX);
      get_engine(L)->trackObject();
    }
  }

  void unrefTimer()
  {
    if (m_runningRef != LUA_REFNIL) {
      luaL_unref(L, LUA_REGISTRYINDEX, m_runningRef);
      m_runningRef = LUA_REFNIL;
      get_engine(L)->untrackObject();
    }
  }

private:
  // Reference used to keep the timer alive (so it's not garbage
  // collected) when it's running.
  int m_runningRef = LUA_REFNIL;

  lua_State* L;
};

LUA_FUNCTION(Timer_new)
{
  auto timer = push_new<Timer>(L, L);

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
                engine_print(L, s);
            }
          }
          else {
            lua_pop(L, 1); // Pop the value which should have been a function
          }
          lua_pop(L, 1); // Pop timer
        }
        catch (const std::exception& ex) {
          engine_print(L, ex.what());
        }
      });
    }
    lua_pop(L, 1);
  }
  return 1;
}

LUA_FUNCTION(Timer_gc)
{
  auto obj = get_obj<Timer>(L, 1);
  obj->~Timer();
  return 0;
}

LUA_FUNCTION(Timer_start)
{
  auto obj = get_obj<Timer>(L, 1);
  obj->start();
  obj->refTimer();
  return 0;
}

LUA_FUNCTION(Timer_stop)
{
  auto obj = get_obj<Timer>(L, 1);
  obj->stop();
  obj->unrefTimer();
  return 0;
}

LUA_FUNCTION(Timer_get_interval)
{
  const auto timer = get_obj<Timer>(L, 1);
  lua_pushnumber(L, double(timer->interval()) / 1000.0);
  return 1;
}

LUA_FUNCTION(Timer_set_interval)
{
  const auto timer = get_obj<Timer>(L, 1);
  int interval = int(lua_tonumber(L, 2) * 1000.0);
  timer->setInterval(std::max(1, interval));
  return 1;
}

LUA_FUNCTION(Timer_get_isRunning)
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
DEF_ITERATOR_PAIRS(Timer);
} // anonymous namespace

DEF_MTNAME(Timer);

LUA_FUNCTION(register_timer_class)
{
  REG_CLASS(L, Timer);
  REG_CLASS_NEW(L, Timer);
  REG_CLASS_PROPERTIES(L, Timer);
  return 0;
}

}} // namespace app::script
