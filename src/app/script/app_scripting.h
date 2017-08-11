// Aseprite
// Copyright (C) 2001-2017  David Capello
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
}

namespace app {
  class Document;
  class ImageWrap;
  class SpriteWrap;

  class AppScripting : public script::Engine {
    typedef std::map<doc::ObjectId, SpriteWrap*> Sprites;

  public:
    AppScripting(script::EngineDelegate* delegate);

    SpriteWrap* wrapSprite(app::Document* doc);

  protected:
    void onAfterEval(bool err) override;

  private:
    void destroyWrappers();
    Sprites m_sprites;
  };

  AppScripting* unwrap_engine(script::Context& ctx);

  void push_new_point(script::Context& ctx, const gfx::Point& pt);
  void push_new_rectangle(script::Context& ctx, const gfx::Rect& rc);
  void push_new_size(script::Context& ctx, const gfx::Size& rc);
  void push_new_selection(script::Context& ctx, SpriteWrap* spriteWrap);

  gfx::Point convert_args_into_point(script::Context& ctx);
  gfx::Rect convert_args_into_rectangle(script::Context& ctx);
  gfx::Size convert_args_into_size(script::Context& ctx);

} // namespace app

#endif
