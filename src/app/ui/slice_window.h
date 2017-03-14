// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SLICE_WINDOW_H_INCLUDED
#define APP_UI_SLICE_WINDOW_H_INCLUDED
#pragma once

#include "doc/anidir.h"
#include "doc/frame.h"
#include "doc/user_data.h"

#include "slice_properties.xml.h"

namespace doc {
  class Slice;
  class Sprite;
}

namespace app {

  class SliceWindow : protected app::gen::SliceProperties {
  public:
    SliceWindow(const doc::Sprite* sprite,
                const doc::Slice* slice,
                const doc::frame_t frame);

    bool show();

    std::string nameValue() const;
    gfx::Rect boundsValue() const;
    gfx::Rect centerValue() const;
    gfx::Point pivotValue() const;
    const doc::UserData& userDataValue() { return m_userData; }

  private:
    void onCenterChange();
    void onPivotChange();
    void onPopupUserData();

    doc::UserData m_userData;
  };

}

#endif
