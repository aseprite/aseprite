// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_MOVING_SLICE_STATE_H_INCLUDED
#define APP_UI_EDITOR_MOVING_SLICE_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/editor_hit.h"
#include "app/ui/editor/standby_state.h"
#include "app/ui/editor/pixels_movement.h"
#include "doc/frame.h"
#include "doc/selected_objects.h"
#include "doc/slice.h"

namespace app {
  class Editor;

  class MovingSliceState : public StandbyState {
  public:
    MovingSliceState(Editor* editor,
                     ui::MouseMessage* msg,
                     const EditorHit& hit,
                     const doc::SelectedObjects& selectedSlices,
                     PixelsMovementPtr pixelsMovement);

    LeaveAction onLeaveState(Editor *editor, EditorState *newState) override;
    bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;

    bool requireBrushPreview() override { return false; }

  private:
    struct Item {
      doc::Slice* slice;
      doc::SliceKey oldKey;
      doc::SliceKey newKey;
    };

    Item getItemForSlice(doc::Slice* slice);
    gfx::Rect selectedSlicesBounds() const;

    doc::frame_t m_frame;
    EditorHit m_hit;
    gfx::Point m_mouseStart;
    std::vector<Item> m_items;
    // Helper member to move/translate the pixels under the slices.
    PixelsMovementPtr m_pixelsMovement;
  };

} // namespace app

#endif
