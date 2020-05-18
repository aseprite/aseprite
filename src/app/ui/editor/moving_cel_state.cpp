// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
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

  DocRange range = App::instance()->timeline()->range();
  if (!range.enabled()) {
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
  for (Cel* cel : get_unlocked_unique_cels(editor->sprite(), range2)) {
    Layer* layer = cel->layer();
    ASSERT(layer);

    if (layer && layer->isMovable() && !layer->isBackground())
      m_celList.push_back(cel);
  }
}

MovingCelState::MovingCelState(Editor* editor,
                               MouseMessage* msg,
                               const HandleType handle,
                               const MovingCelCollect& collect)
  : m_reader(UIContext::instance(), 500)
  , m_cel(nullptr)
  , m_celList(collect.celList())
  , m_celOffset(0.0, 0.0)
  , m_celScale(1.0, 1.0)
  , m_hasReference(false)
  , m_scaled(false)
  , m_handle(handle)
{
  ContextWriter writer(m_reader);
  Doc* document = editor->document();
  ASSERT(!m_celList.empty());

  m_cel = collect.mainCel();
  if (m_cel)
    m_celMainSize = m_cel->boundsF().size();

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

  m_cursorStart = editor->screenToEditorF(msg->position());
  editor->captureMouse();

  // Hide the mask (temporarily, until mouse-up event)
  m_maskVisible = document->isMaskVisible();
  if (m_maskVisible) {
    document->setMaskVisible(false);
    document->generateMaskBoundaries();
  }
}

bool MovingCelState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  Doc* document = editor->document();
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

  if (modified) {
    {
      ContextWriter writer(m_reader, 1000);
      Tx tx(writer.context(), "Cel Movement", ModifyDocument);
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
  const gfx::Point mousePos = editor->autoScroll(msg, AutoScroll::MouseDir);
  const gfx::PointF newCursorPos = editor->screenToEditorF(mousePos);

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

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
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
  gfx::PointF pos = m_cursorStart - gfx::PointF(editor->mainTilePosition());

  if (m_hasReference) {
    if (m_scaled && m_cel) {
      StatusBar::instance()->setStatusText(
        0,
        fmt::format(
          ":pos: {:.2f} {:.2f} :offset: {:.2f} {:.2f} :size: {:.2f}% {:.2f}%",
          pos.x, pos.y,
          m_celOffset.x, m_celOffset.y,
          100.0*m_celScale.w*m_celMainSize.w/m_cel->image()->width(),
          100.0*m_celScale.h*m_celMainSize.h/m_cel->image()->height()));
    }
    else {
      StatusBar::instance()->setStatusText(
        0,
        fmt::format(
          ":pos: {:.2f} {:.2f} :offset: {:.2f} {:.2f}",
          pos.x, pos.y,
          m_celOffset.x, m_celOffset.y));
    }
  }
  else {
    gfx::Point intOffset = intCelOffset();
    StatusBar::instance()->setStatusText(
      0,
      fmt::format(":pos: {:3d} {:3d} :offset: {:3d} {:3d}",
                  int(pos.x), int(pos.y),
                  intOffset.x, intOffset.y));
  }

  return true;
}

gfx::Point MovingCelState::intCelOffset() const
{
  return gfx::Point(int(std::round(m_celOffset.x)),
                    int(std::round(m_celOffset.y)));
}

} // namespace app
