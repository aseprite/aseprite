// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPTING_H_INCLUDED
#define APP_SCRIPTING_H_INCLUDED
#pragma once

#include "doc/object_id.h"
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

} // namespace app

#endif
