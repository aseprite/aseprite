// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REMAP_COLORS_H_INCLUDED
#define APP_CMD_REMAP_COLORS_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/remap.h"

namespace app { namespace cmd {
using namespace doc;

class RemapColors : public Cmd,
                    public WithSprite {
public:
  RemapColors(Sprite* sprite, const Remap& remap);

protected:
  void onExecute() override;
  void onUndo() override;
  size_t onMemSize() const override { return sizeof(*this) + m_remap.getMemSize(); }

private:
  void incrementVersions(Sprite* spr);

  Remap m_remap;
};

}} // namespace app::cmd

#endif
