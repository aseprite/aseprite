// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_SET_FRAME_TAG_COLOR_H_INCLUDED
#define APP_CMD_SET_FRAME_TAG_COLOR_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_frame_tag.h"
#include "doc/color.h"

namespace app {
namespace cmd {
  using namespace doc;

  class SetFrameTagColor : public Cmd
                         , public WithFrameTag {
  public:
    SetFrameTagColor(FrameTag* tag, doc::color_t color);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    doc::color_t m_oldColor;
    doc::color_t m_newColor;
  };

} // namespace cmd
} // namespace app

#endif
