// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_SPRITE_TILE_MANAGEMENT_PLUGIN_H_INCLUDED
#define APP_CMD_SET_SPRITE_TILE_MANAGEMENT_PLUGIN_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"

#include <string>

namespace app { namespace cmd {
using namespace doc;

class SetSpriteTileManagementPlugin : public Cmd,
                                      public WithSprite {
public:
  SetSpriteTileManagementPlugin(Sprite* sprite, const std::string& value);

protected:
  void onExecute() override;
  void onUndo() override;
  void onFireNotifications() override;
  size_t onMemSize() const override
  {
    return sizeof(*this) + m_oldValue.size() + m_newValue.size();
  }

private:
  std::string m_oldValue;
  std::string m_newValue;
};

}} // namespace app::cmd

#endif
