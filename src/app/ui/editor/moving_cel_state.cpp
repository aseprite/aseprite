// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/moving_cel_state.h"

#include "app/app.h"
#include "app/cmd/set_cel_bounds.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/doc_api.h"
#include "app/doc_range.h"
#include "app/tx.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline/timeline.h"
#include "app/ui_context.h"
#include "app/util/range_utils.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "fmt/format.h"
#include "ui/message.h"

#include <algorithm>
#include <cmath>

namespace app {

using namespace ui;

MovingCelCollect::MovingCelCollect(Editor* editor, Layer* layer)
  : m_mainCel(nullptr)
{
  ASSERT(editor);

  if (layer && layer->isImage())
    m_mainCel = layer->cel(editor->frame());

  Timeline* timeline = App::instance()->timeline();
  DocRange range = timeline->range();
  if (!range.enabled() ||
      !timeline->isVisible()) {
    range.startRange(editor->layer(), editor->frame(), DocRange::kCels);
    range.endRange(editor->layer(), editor->frame());
  }

  DocRange range2 = range;
  for (Layer* layer : range.selectedLayers()) {
    if (layer && layer->isGroup()) {
      LayerList childrenList;
      static_cast<LayerGroup*>(layer)->allLayers(childrenList);

      SelectedLayers selChildren;
      for (auto layer : childrenList)
        selChildren.insert(layer);

      range2.selectLayers(selChildren);
    }
  }

  // Record start positions of all cels in selected range
  for (Cel* cel : get_unique_cels_to_move_cel(editor->sprite(), range2)) {
    Layer* layer = cel->layer();
    ASSERT(layer);

    if (layer && layer->isMovable() && !layer->isBackground())
      m_celList.push_back(cel);
  }
}

MovingCelState::MovingCelState(Editor* editor,
                               const MouseMessage* msg,
                               const HandleType handle,
                               const MovingCelCollect& collect)
  : m_reader(UIContext::instance(), 500)
  , m_delayedMouseMove(this, editor, 5)
  , m_cel(nullptr)
  , m_celList(collect.celList())
  , m_celOffset(0.0, 0.0)
  , m_celScale(1.0, 1.0)
  , m_handle(handle)
  , m_editor(editor)
{
  ContextWriter writer(m_reader);
  Doc* document = editor->document();
  ASSERT(!m_celList.empty());

  m_cel = collect.mainCel();
  if (m_cel) {
    if (m_cel->data()->hasBoundsF())
      m_celMainSize = m_cel->boundsF().size();
    else
      m_celMainSize = gfx::SizeF(m_cel->bounds().size());
  }

  // Record start positions of all cels in selected range
  for (Cel* cel : m_celList) {
    Layer* layer = cel->layer();
    ASSERT(layer);

    if (layer && layer->isMovable() && !layer->isBackground()) {
      if (layer->isReference()) {
        m_celStarts.push_back(cel->boundsF());
        m_hasReference = true;
      }
      else
        m_celStarts.push_back(cel->bounds());
    }
  }

  // Hook BeforeCommandExecution signal so we know if the user wants
  // to execute other command, so we can drop pixels.
  m_ctxConn = UIContext::instance()->BeforeCommandExecution.connect(
    &MovingCelState::onBeforeCommandExecution, this);

  m_cursorStart = editor->screenToEditorF(msg->position());
  editor->captureMouse();

  // Hide the mask (temporarily, until mouse-up event)
  m_maskVisible = document->isMaskVisible();
  if (m_maskVisible) {
    document->setMaskVisible(false);
    document->generateMaskBoundaries();
  }

  m_delayedMouseMove.onMouseDown(msg);
}

void MovingCelState::onBeforePopState(Editor* editor)
{
  m_ctxConn.disconnect();
  StandbyState::onBeforePopState(editor);
}

bool MovingCelState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  m_delayedMouseMove.onMouseUp(msg);

  Doc* document = editor->document();
  bool modified = restoreCelStartPosition();

  if (modified) {
    {
      ContextWriter writer(m_reader, 1000);
      Tx tx(writer, "Cel Movement", ModifyDocument);
      DocApi api = document->getApi(tx);
      gfx::Point intOffset = intCelOffset();

      // And now we move the cel (or all selected range) to the new position.
      for (Cel* cel : m_celList) {
        // Change reference layer with subpixel precision
        if (cel->layer()->isReference()) {
          gfx::RectF celBounds = cel->boundsF();
          celBounds.x += m_celOffset.x;
          celBounds.y += m_celOffset.y;
          if (m_scaled) {
            celBounds.w *= m_celScale.w;
            celBounds.h *= m_celScale.h;
          }
          tx(new cmd::SetCelBoundsF(cel, celBounds));
        }
        else {
          api.setCelPosition(writer.sprite(), cel,
                             cel->x() + intOffset.x,
                             cel->y() + intOffset.y);
        }
      }

      // Move selection if it was visible
      if (m_maskVisible) {
        // TODO Moving the mask when we move a ref layer (e.g. by
        //      m_celOffset=(0.5,0.5)) will not move the final
        //      position of the mask (so the ref layer is moved and
        //      the mask isn't).

        api.setMaskPosition(document->mask()->bounds().x + intOffset.x,
                            document->mask()->bounds().y + intOffset.y);
      }

      tx.commit();
    }

    // Redraw all editors. We've to notify all views about this
    // general update because MovingCelState::onMouseMove() redraws
    // only the cels in the current editor. And at this point we'd
    // like to update all the editors.
    document->notifyGeneralUpdate();
  }
  // Just a click in the current layer
  else if (!m_moved & !m_scaled) {
    // Deselect the whole range if we are in "Auto Select Layer"
    if (editor->isAutoSelectLayer()) {
      App::instance()->timeline()->clearAndInvalidateRange();
    }
  }

  // Restore the mask visibility.
  if (m_maskVisible) {
    document->setMaskVisible(m_maskVisible);
    document->generateMaskBoundaries();
  }

  editor->backToPreviousState();
  editor->releaseMouse();
  return true;
}

bool MovingCelState::onMouseMove(Editor* editor, MouseMessage* msg)
{
  m_delayedMouseMove.onMouseMove(msg);

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

void MovingCelState::onCommitMouseMove(Editor* editor,
                                       const gfx::PointF& newCursorPos)
{
  switch (m_handle) {

    case MovePixelsHandle:
      m_celOffset = newCursorPos - m_cursorStart;
      if (int(editor->getCustomizationDelegate()
              ->getPressedKeyAction(KeyContext::TranslatingSelection) & KeyAction::LockAxis)) {
        if (ABS(m_celOffset.x) < ABS(m_celOffset.y)) {
          m_celOffset.x = 0;
        }
        else {
          m_celOffset.y = 0;
        }
      }
      if (!m_moved && intCelOffset() != gfx::Point(0, 0))
        m_moved = true;
      break;

    case ScaleSEHandle: {
      gfx::PointF delta(newCursorPos - m_cursorStart);
      m_celScale.w = 1.0 + (delta.x / m_celMainSize.w);
      m_celScale.h = 1.0 + (delta.y / m_celMainSize.h);
      if (m_celScale.w < 1.0/m_celMainSize.w) m_celScale.w = 1.0/m_celMainSize.w;
      if (m_celScale.h < 1.0/m_celMainSize.h) m_celScale.h = 1.0/m_celMainSize.h;

      if (int(editor->getCustomizationDelegate()
              ->getPressedKeyAction(KeyContext::ScalingSelection) & KeyAction::MaintainAspectRatio)) {
        m_celScale.w = m_celScale.h = std::max(m_celScale.w, m_celScale.h);
      }

      m_scaled = true;
      break;
    }
  }

  gfx::Point intOffset = intCelOffset();

  for (size_t i=0; i<m_celList.size(); ++i) {
    Cel* cel = m_celList[i];
    gfx::RectF celBounds = m_celStarts[i];

    if (cel->layer()->isReference()) {
      celBounds.x += m_celOffset.x;
      celBounds.y += m_celOffset.y;
      m_moved = true;
      if (m_scaled) {
        celBounds.w *= m_celScale.w;
        celBounds.h *= m_celScale.h;
      }
      cel->setBoundsF(celBounds);
    }
    else {
      celBounds.x += intOffset.x;
      celBounds.y += intOffset.y;
      cel->setBounds(gfx::Rect(celBounds));
    }
  }

  // Redraw the new cel position.
  editor->invalidate();

  // Redraw status bar with the new position of cels (without this the
  // previous position before this onCommitMouseMove() is still
  // displayed in the screen).
  editor->updateStatusBar();
}

bool MovingCelState::onKeyDown(Editor* editor, KeyMessage* msg)
{
  // Do not call StandbyState::onKeyDown() so we don't start a
  // straight line when we are moving the cel with Ctrl and Shift key
  // is pressed.
  //
  // TODO maybe MovingCelState shouldn't be a StandbyState (the same
  // for several other states)
  return false;
}

bool MovingCelState::onUpdateStatusBar(Editor* editor)
{
  gfx::PointF pos = m_celOffset + m_cursorStart - gfx::PointF(editor->mainTilePosition());
  gfx::RectF fullBounds = calcFullBounds();
  std::string buf;

  if (m_hasReference) {
    buf = fmt::format(":pos: {:.2f} {:.2f}", pos.x, pos.y);
    if (m_scaled && m_cel) {
      buf += fmt::format(
        " :start: {:.2f} {:.2f}"
        " :size: {:.2f} {:.2f} [{:.2f}% {:.2f}%]",
        m_cel->boundsF().x,
        m_cel->boundsF().y,
        m_celScale.w*m_celMainSize.w,
        m_celScale.h*m_celMainSize.h,
        100.0*m_celScale.w*m_celMainSize.w/m_cel->image()->width(),
        100.0*m_celScale.h*m_celMainSize.h/m_cel->image()->height());
    }
    else {
      buf += fmt::format(
        " :start: {:.2f} {:.2f} :size: {:.2f} {:.2f}"
        " :delta: {:.2f} {:.2f}",
        fullBounds.x, fullBounds.y,
        fullBounds.w, fullBounds.h,
        m_celOffset.x, m_celOffset.y);
    }
  }
  else {
    gfx::Point intOffset = intCelOffset();
    fullBounds.floor();
    buf = fmt::format(
      ":pos: {} {}"
      " :start: {} {} :size: {} {}"
      " :delta: {} {}",
      int(pos.x), int(pos.y),
      int(fullBounds.x), int(fullBounds.y),
      int(fullBounds.w), int(fullBounds.h),
      intOffset.x, intOffset.y);
  }

  StatusBar::instance()->setStatusText(0, buf);
  return true;
}

gfx::Point MovingCelState::intCelOffset() const
{
  return gfx::Point(int(std::round(m_celOffset.x)),
                    int(std::round(m_celOffset.y)));
}

gfx::RectF MovingCelState::calcFullBounds() const
{
  gfx::RectF bounds;
  for (Cel* cel : m_celList) {
    if (cel->data()->hasBoundsF())
      bounds |= cel->boundsF();
    else
      bounds |= gfx::RectF(cel->bounds()).floor();
  }
  return bounds;
}

bool MovingCelState::restoreCelStartPosition() const
{
  bool modified = false;

  // Here we put back all cels into their original coordinates (so we
  // can add the undo information from the start position).
  for (size_t i=0; i<m_celList.size(); ++i) {
    Cel* cel = m_celList[i];
    const gfx::RectF& celStart = m_celStarts[i];

    if (cel->layer()->isReference()) {
      if (cel->boundsF() != celStart) {
        cel->setBoundsF(celStart);
        modified = true;
      }
    }
    else {
      if (cel->bounds() != gfx::Rect(celStart)) {
        cel->setBounds(gfx::Rect(celStart));
        modified = true;
      }
    }
  }
  return modified;
}

void MovingCelState::onBeforeCommandExecution(CommandExecutionEvent& ev)
{
  if (ev.command()->id() == CommandId::Undo() ||
      ev.command()->id() == CommandId::Redo() ||
      ev.command()->id() == CommandId::Cancel()) {
    restoreCelStartPosition();
    Doc* document = m_editor->document();
    // Restore the mask visibility.
    if (m_maskVisible) {
      document->setMaskVisible(m_maskVisible);
      document->generateMaskBoundaries();
    }
    m_editor->backToPreviousState();
    m_editor->releaseMouse();
    m_editor->invalidate();
  }
  ev.cancel();
}

} // namespace app
