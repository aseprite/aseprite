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
#include "script/engine.h"

#include <map>

namespace doc {
  class Image;
  class Sprite;
}

namespace app {
  class Site;

  class AppScripting : public script::Engine {
  public:
    AppScripting(script::EngineDelegate* delegate);
  };

  AppScripting* get_engine(script::Context& ctx);

  void push_image(script::Context& ctx, doc::Image* image);
  void push_new_point(script::Context& ctx, const gfx::Point& pt);
  void push_new_rectangle(script::Context& ctx, const gfx::Rect& rc);
  void push_new_size(script::Context& ctx, const gfx::Size& rc);
  void push_site(script::Context& ctx, app::Site& site);
  void push_sprite(script::Context& ctx, doc::Sprite* sprite);
  void push_sprite_selection(script::Context& ctx, doc::Sprite* sprite);

  gfx::Point convert_args_into_point(script::Context& ctx);
  gfx::Rect convert_args_into_rectangle(script::Context& ctx);
  gfx::Size convert_args_into_size(script::Context& ctx);

} // namespace app

#endif
