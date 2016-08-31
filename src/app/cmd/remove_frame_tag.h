// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_REMOVE_FRAME_TAG_H_INCLUDED
#define APP_CMD_REMOVE_FRAME_TAG_H_INCLUDED
#pragma once

#include "app/cmd/add_frame_tag.h"

namespace app {
namespace cmd {
  using namespace doc;

  class RemoveFrameTag : public AddFrameTag {
  public:
    RemoveFrameTag(Sprite* sprite, FrameTag* tag);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
  };

} // namespace cmd
} // namespace app

#endif
