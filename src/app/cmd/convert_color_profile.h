// Aseprite
// Copyright (C) 2018-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_CONVERT_COLOR_PROFILE_H_INCLUDED
#define APP_CMD_CONVERT_COLOR_PROFILE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/sequence.h"
#include "app/cmd/with_sprite.h"
#include "gfx/color_space.h"

namespace gfx {
class ColorSpace;
}

namespace doc {
class Image;
class Palette;
} // namespace doc

namespace app { namespace cmd {

class ConvertColorProfile : public Cmd,
                            public WithSprite {
public:
  CMDTYPE('t', 'o', 'C', 's');

  ConvertColorProfile(doc::Sprite* sprite, const gfx::ColorSpaceRef& newCS);

protected:
  void onExecute(Context* ctx) override;
  void onUndo(Context* ctx) override;
  void onRedo(Context* ctx) override;
  size_t onMemSize() const override { return sizeof(*this) + m_seq.memSize(); }

private:
  CmdSequence m_seq;
};

// Converts the sprite to the new color profile without undo information.
// TODO how to merge this function with cmd::ConvertColorProfile
void convert_color_profile(doc::Sprite* sprite, const gfx::ColorSpaceRef& newCS);

void convert_color_profile(doc::Image* image,
                           doc::Palette* palette,
                           const gfx::ColorSpaceRef& oldCS,
                           const gfx::ColorSpaceRef& newCS);

}} // namespace app::cmd

#endif
