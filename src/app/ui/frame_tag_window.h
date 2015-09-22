// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_FRAME_TAG_WINDOW_H_INCLUDED
#define APP_UI_FRAME_TAG_WINDOW_H_INCLUDED
#pragma once

#include "app/ui/color_button.h"
#include "doc/anidir.h"
#include "doc/frame.h"

#include "frame_tag_properties.xml.h"

namespace doc {
  class FrameTag;
  class Sprite;
}

namespace ui {
  class Splitter;
}

namespace app {

  class FrameTagWindow : protected app::gen::FrameTagProperties {
  public:
    FrameTagWindow(const doc::Sprite* sprite, const doc::FrameTag* frameTag);

    bool show();

    std::string nameValue();
    void rangeValue(doc::frame_t& from, doc::frame_t& to);
    doc::color_t colorValue();
    doc::AniDir aniDirValue();

    private:
      const doc::Sprite* m_sprite;
  };

}

#endif
