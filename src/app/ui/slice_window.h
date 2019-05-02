// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SLICE_WINDOW_H_INCLUDED
#define APP_UI_SLICE_WINDOW_H_INCLUDED
#pragma once

#include "doc/anidir.h"
#include "doc/frame.h"
#include "doc/selected_objects.h"
#include "doc/user_data.h"

#include "slice_properties.xml.h"

namespace doc {
  class Slice;
  class Sprite;
}

namespace app {

  class SliceWindow : protected app::gen::SliceProperties {
  public:
    enum Mods {
      kNone          = 0x0000,
      kName          = 0x0001,
      kBoundsX       = 0x0002,
      kBoundsY       = 0x0004,
      kBoundsW       = 0x0008,
      kBoundsH       = 0x0010,
      kCenterX       = 0x0020,
      kCenterY       = 0x0040,
      kCenterW       = 0x0080,
      kCenterH       = 0x0100,
      kPivotX        = 0x0200,
      kPivotY        = 0x0400,
      kUserData      = 0x0800,
      kAll           = 0xffff,
    };

    SliceWindow(const doc::Sprite* sprite,
                const doc::SelectedObjects& slices,
                const doc::frame_t frame);

    bool show();

    std::string nameValue() const;
    gfx::Rect boundsValue() const;
    gfx::Rect centerValue() const;
    gfx::Point pivotValue() const;
    const doc::UserData& userDataValue() { return m_userData; }

    Mods modifiedFields() const { return m_mods; }

  private:
    void onCenterChange();
    void onPivotChange();
    void onPopupUserData();
    void onModifyField(ui::Entry* entry, const Mods mods);

    doc::UserData m_userData;

    // Flags used to know what specific entry/checkbox was modified
    // when we edit multiple-slices in the same property dialog. In
    // this way we know what field modify of each slice in
    // SlicePropertiesCommand::onExecute().
    Mods m_mods;
  };

}

#endif
