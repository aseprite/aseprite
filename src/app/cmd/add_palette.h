// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_ADD_PALETTE_H_INCLUDED
#define APP_CMD_ADD_PALETTE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/frame.h"

#include <sstream>

namespace doc {
class Palette;
class Sprite;
} // namespace doc

namespace app { namespace cmd {
using namespace doc;

class AddPalette : public Cmd,
                   public WithSprite {
public:
  AddPalette(Sprite* sprite, Palette* pal);

protected:
  void onExecute() override;
  void onUndo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_size; }

private:
  size_t m_size;
  std::stringstream m_stream;
  frame_t m_frame;
};

}} // namespace app::cmd

#endif // CMD_ADD_PALETTE_H_INCLUDED
