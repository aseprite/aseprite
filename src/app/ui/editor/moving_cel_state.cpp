// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/moving_cel_state.h"

#include "app/app.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/document_range.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_customization_delegate.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "app/ui_context.h"
#include "app/transaction.h"
#include "app/util/range_utils.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/message.h"

namespace app {

using namespace ui;

MovingCelState::MovingCelState(Editor* editor, MouseMessage* msg)
  : m_canceled(false)
{
  Document* document = editor->document();
  LayerImage* layer = static_cast<LayerImage*>(editor->layer());
  ASSERT(layer->isImage());

  m_cel = layer->cel(editor->frame());
  ASSERT(m_cel); // The cel cannot be null
  if (m_cel) {
    m_celStart = m_cel->position();
  }
  else {
    m_celStart = gfx::Point(0, 0);
  }
  m_celNew = m_celStart;

  m_mouseStart = editor->screenToEditor(msg->position());
  editor->captureMouse();

  // Hide the mask (temporarily, until mouse-up event)
  m_maskVisible = document->isMaskVisible();
  if (m_maskVisible) {
    document->setMaskVisible(false);
    document->generateMaskBoundaries();
  }
}

MovingCelState::~MovingCelState()
{
}

bool MovingCelState::onMouseUp(Editor* editor, MouseMessage* msg)
{
  Document* document = editor->document();

  // Here we put back the cel into its original coordinate (so we can
  // add an undoer before).
  if (m_celStart != m_celNew) {
    // Put the cel in the original position.
    if (m_cel)
      m_cel->setPosition(m_celStart);

    // If the user didn't cancel the operation...
    if (!m_canceled) {
      ContextWriter writer(UIContext::instance(), 500);
      Transaction transaction(writer.context(), "Cel Movement", ModifyDocument);
      DocumentApi api = document->getApi(transaction);

      // And now we move the cel (or all selected range) to the new position.
      gfx::Point delta = m_celNew - m_celStart;

      DocumentRange range = App::instance()->getMainWindow()->getTimeline()->range();
      if (!range.enabled())
        range = DocumentRange(m_cel);

      for (Cel* cel : get_unique_cels(writer.sprite(), range)) {
        Layer* layer = cel->layer();
        ASSERT(layer);
        if (layer && layer->isMovable() && !layer->isBackground()) {
          api.setCelPosition(writer.sprite(), cel, cel->x()+delta.x, cel->y()+delta.y);
        }
      }

      // Move selection if it was visible
      if (m_maskVisible)
        api.setMaskPosition(document->mask()->bounds().x + delta.x,
                            document->mask()->bounds().y + delta.y);

      transaction.commit();
    }

    // Redraw all editors. We've to notify all views about this
    // general update because MovingCelState::onMouseMove() redraws
    // only the current cel in the current editor. And at this point
    // we might have moved several cels (and we've to update all the
    // editors).
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
  gfx::Point newCursorPos = editor->screenToEditor(msg->position());
  gfx::Point delta = newCursorPos - m_mouseStart;

  if (int(editor->getCustomizationDelegate()->getPressedKeyAction(KeyContext::MovingPixels) & KeyAction::LockAxis)) {
    if (ABS(delta.x) < ABS(delta.y)) {
      delta.x = 0;
    }
    else {
      delta.y = 0;
    }
  }

  m_celNew = m_celStart + delta;
  if (m_cel)
    m_cel->setPosition(m_celNew);

  // Redraw the new cel position.
  editor->invalidate();

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingCelState::onUpdateStatusBar(Editor* editor)
{
  StatusBar::instance()->setStatusText
    (0,
     "Pos %3d %3d Offset %3d %3d",
     (int)m_celNew.x,
     (int)m_celNew.y,
     (int)(m_celNew.x - m_celStart.x),
     (int)(m_celNew.y - m_celStart.y));

  return true;
}

} // namespace app
