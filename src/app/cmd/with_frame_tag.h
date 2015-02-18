// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_CMD_WITH_FRAME_TAG_H_INCLUDED
#define APP_CMD_WITH_FRAME_TAG_H_INCLUDED
#pragma once

#include "doc/object_id.h"

namespace doc {
  class FrameTag;
}

namespace app {
namespace cmd {
  using namespace doc;

  class WithFrameTag {
  public:
    WithFrameTag(FrameTag* tag);
    FrameTag* frameTag();

  private:
    ObjectId m_frameTagId;
  };

} // namespace cmd
} // namespace app

#endif
