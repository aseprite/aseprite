// Aseprite
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

#include "doc/object_id.h"
#include "gfx/fwd.h"

#include <cstdio>
#include <string>
#include <map>

struct lua_State;

namespace doc {
  class Cel;
  class Sprite;
}

namespace app {
  class Site;

namespace script {

  class EngineDelegate {
  public:
    virtual ~EngineDelegate() { }
    virtual void onConsolePrint(const char* text) = 0;
  };

  class StdoutEngineDelegate : public EngineDelegate {
  public:
    void onConsolePrint(const char* text) override {
      std::printf("%s\n", text);
    }
  };

  class Engine {
  public:
    Engine(EngineDelegate* delegate);
    ~Engine();

    void printLastResult();
    bool evalCode(const std::string& code,
                  const std::string& filename = std::string());
    bool evalFile(const std::string& filename);

  private:
    lua_State* L;
    EngineDelegate* m_delegate;
    bool m_printLastResult;
  };

  void push_sprite_selection(lua_State* L, doc::Sprite* sprite);
  void push_cel_image(lua_State* L, doc::Cel* cel);

  gfx::Point convert_args_into_point(lua_State* L, int index);
  gfx::Rect convert_args_into_rect(lua_State* L, int index);
  gfx::Size convert_args_into_size(lua_State* L, int index);

} // namespace script
} // namespace app

#endif
