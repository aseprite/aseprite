// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_PIXEL_FORMAT_H_INCLUDED
#define APP_CMD_SET_PIXEL_FORMAT_H_INCLUDED
#pragma once

#include "app/cmd_sequence.h"
#include "app/cmd/with_sprite.h"
#include "doc/pixel_format.h"
#include "doc/dithering_method.h"

namespace doc {
  class Sprite;
}

namespace app {
namespace cmd {
  using namespace doc;

  class SetPixelFormat : public Cmd
                       , public WithSprite {
  public:
    SetPixelFormat(Sprite* sprite,
      PixelFormat newFormat,
      DitheringMethod dithering);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) + m_seq.memSize();
    }

  private:
    void setFormat(PixelFormat format);

    PixelFormat m_oldFormat;
    PixelFormat m_newFormat;
    DitheringMethod m_dithering;
    CmdSequence m_seq;
  };

} // namespace cmd
} // namespace app

#endif
