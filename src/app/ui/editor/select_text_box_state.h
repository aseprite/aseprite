// Aseprite
// Copyright (c) 2022-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_SELECT_TEXT_BOX_STATE_H_INCLUDED
#define APP_UI_EDITOR_SELECT_TEXT_BOX_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/select_box_state.h"

namespace app {

class SelectTextBoxState : public SelectBoxState
                         , public SelectBoxDelegate {
public:
  SelectTextBoxState(Editor* editor, ui::MouseMessage* msg);

  bool isTemporalState() const override { return true; }

  bool onUpdateStatusBar(Editor* editor) override;

  // SelectBoxDelegate impl
  void onChangeRectangle(const gfx::Rect& rect) override;
  void onQuickboxEnd(Editor* editor, const gfx::Rect& rect, ui::MouseButton button) override;
  void onQuickboxCancel(Editor* editor) override;

private:
  Editor* m_editor;
};

} // namespace app

#endif  // APP_UI_EDITOR_DRAWING_STATE_H_INCLUDED
