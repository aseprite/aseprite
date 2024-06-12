// Aseprite
// Copyright (c) 2022-2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/select_text_box_state.h"

#include "app/ui/editor/editor.h"
#include "app/ui/editor/writing_text_state.h"
#include "app/ui/status_bar.h"
#include "fmt/format.h"

namespace app {

SelectTextBoxState::SelectTextBoxState(Editor* editor,
                                       ui::MouseMessage* msg)
  : SelectBoxState(this, editor->sprite()->bounds(),
                   Flags(int(Flags::QuickBox) |
                         int(Flags::DarkOutside)))
  , m_editor(editor)
{
  onMouseDown(editor, msg);
}

bool SelectTextBoxState::onUpdateStatusBar(Editor* editor)
{
  gfx::PointF spritePos =
    editor->screenToEditorF(editor->mousePosInDisplay())
    - gfx::PointF(editor->mainTilePosition());

  const gfx::Rect bounds = getBoxBounds();
  const std::string buf = fmt::format(
    ":pos: {} {}"
    " :start: {} {} :size: {} {}",
    int(spritePos.x), int(spritePos.y),
    int(bounds.x), int(bounds.y),
    int(bounds.w), int(bounds.h));

  StatusBar::instance()->setStatusText(0, buf);
  return true;
}

void SelectTextBoxState::onChangeRectangle(const gfx::Rect&)
{
  onUpdateStatusBar(m_editor);
}

void SelectTextBoxState::onQuickboxEnd(Editor* editor,
                                       const gfx::Rect& rect,
                                       ui::MouseButton)
{
  editor->backToPreviousState();

  // A 1x1 rectangle will cancel the operation
  if (rect.w > 1 && rect.h > 1) {
    EditorStatePtr newState = std::make_shared<WritingTextState>(editor, rect);
    editor->setState(newState);
  }
}

void SelectTextBoxState::onQuickboxCancel(Editor* editor)
{
  editor->backToPreviousState();
}

} // namespace app
