// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_SET_FRAME_TAG_NAME_H_INCLUDED
#define APP_CMD_SET_FRAME_TAG_NAME_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_frame_tag.h"

#include <string>

namespace app {
namespace cmd {
  using namespace doc;

  class SetFrameTagName : public Cmd
                        , public WithFrameTag {
  public:
    SetFrameTagName(FrameTag* tag, const std::string& name);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    std::string m_oldName;
    std::string m_newName;
  };

} // namespace cmd
} // namespace app

#endif
