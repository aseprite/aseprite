// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_SCOPED_TOOL_LOOP_FIX_H_INCLUDED
#define APP_UI_EDITOR_SCOPED_TOOL_LOOP_FIX_H_INCLUDED
#pragma once

#include "app/ui/editor/drawing_state.h"
#include "app/ui/editor/editor.h"
#include "app/util/expand_cel_canvas.h"

namespace app {

// Quite a hack to temporarily fix the Sprite active cel that is being
// used in a DrawingState/ToolLoop/ExpandCelCanvas, so the sprite can
// be used in a Image:drawSprite() scripting API call.
class ScopedToolLoopFix {
public:
  ScopedToolLoopFix(doc::Sprite* sprite)
  {
    if (Editor* editor = Editor::activeEditor()) {
      if (editor->sprite() != sprite)
        return;

      if (auto* state = dynamic_cast<DrawingState*>(editor->getState().get())) {
        m_expand = state->expandCelCanvas();
        if (m_expand)
          m_expand->prepareSpriteForScript();
      }
    }
  }

  ~ScopedToolLoopFix()
  {
    if (m_expand)
      m_expand->restoreSpriteForToolLoop();
  }

private:
  ExpandCelCanvas* m_expand = nullptr;
};

} // namespace app

#endif
