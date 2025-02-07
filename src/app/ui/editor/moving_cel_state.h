// Aseprite
// Copyright (C) 2021-2022 Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_MOVING_CEL_STATE_H_INCLUDED
#define APP_UI_EDITOR_MOVING_CEL_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/standby_state.h"

#include "app/context_access.h"
#include "app/ui/editor/delayed_mouse_move.h"
#include "app/ui/editor/handle_type.h"
#include "doc/cel_list.h"

#include <vector>

namespace doc {
class Cel;
}

namespace app {
class Editor;

class MovingCelCollect {
public:
  MovingCelCollect(Editor* editor, Layer* layer);

  bool empty() const { return m_celList.empty(); }

  Cel* mainCel() const { return m_mainCel; }
  const CelList& celList() const { return m_celList; }

private:
  Cel* m_mainCel;
  CelList m_celList;
};

class MovingCelState : public StandbyState,
                       DelayedMouseMoveDelegate {
public:
  MovingCelState(Editor* editor,
                 const ui::MouseMessage* msg,
                 const HandleType handle,
                 const MovingCelCollect& collect);

  virtual void onBeforePopState(Editor* editor) override;
  virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
  virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
  virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
  virtual bool onUpdateStatusBar(Editor* editor) override;

  virtual bool requireBrushPreview() override { return false; }

private:
  gfx::Point intCelOffset() const;
  gfx::RectF calcFullBounds() const;
  void calcPivot();
  bool restoreCelStartPosition() const;
  void snapOffsetToGrid(gfx::Point& offset) const;
  void snapBoundsToGrid(gfx::RectF& celBounds) const;
  // ContextObserver
  void onBeforeCommandExecution(CommandExecutionEvent& ev);

  // DelayedMouseMoveDelegate impl
  void onCommitMouseMove(Editor* editor, const gfx::PointF& spritePos) override;

  ContextReader m_reader;
  DelayedMouseMove m_delayedMouseMove;
  Cel* m_cel;
  CelList m_celList;
  std::vector<gfx::RectF> m_celStarts;
  gfx::RectF m_fullBounds;
  gfx::PointF m_cursorStart;
  gfx::PointF m_pivot;
  gfx::PointF m_pivotOffset;
  gfx::PointF m_celOffset;
  gfx::SizeF m_celMainSize;
  gfx::SizeF m_celScale;
  bool m_maskVisible;
  bool m_hasReference = false;
  bool m_moved = false;
  bool m_scaled = false;
  HandleType m_handle;
  Editor* m_editor;

  obs::scoped_connection m_ctxConn;
};

} // namespace app

#endif // APP_UI_EDITOR_MOVING_CEL_STATE_H_INCLUDED
