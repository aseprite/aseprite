// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_WITH_SPRITE_H_INCLUDED
#define APP_CMD_WITH_SPRITE_H_INCLUDED
#pragma once

#include "app/cmd_serial.h"
#include "doc/object_id.h"

namespace doc {
class Sprite;
}

namespace app { namespace cmd {

class WithSprite {
public:
  WithSprite(doc::Sprite* sprite = nullptr);
  doc::Sprite* sprite();

  void serializeSpriteId(CmdSerial& s)
  {
    s.serializeObjectId(m_spriteId);
    PRINTARGS("serializeSpriteId", m_spriteId);
  }

private:
  doc::ObjectId m_spriteId;
};

}} // namespace app::cmd

#endif
