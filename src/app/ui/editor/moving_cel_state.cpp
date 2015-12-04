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
  ContextWriter writer(UIContext::instance(), 500);
  Document* document = editor->document();
  DocumentRange range = App::instance()->getMainWindow()->getTimeline()->range();
  LayerImage* layer = static_cast<LayerImage*>(editor->layer());
  ASSERT(layer->isImage());

  Cel* currentCel = layer->cel(editor->frame());
  ASSERT(currentCel); // The cel cannot be null

  if (!range.enabled())
    range = DocumentRange(currentCel);

  // Record start positions of all cels in selected range
  for (Cel* cel : get_unique_cels(writer.sprite(), range)) {
    Layer* layer = cel->layer();
    ASSERT(layer);

    if (layer && layer->isMovable() && !layer->isBackground()) {
      m_celList.push_back(cel);
      m_celStarts.push_back(cel->position());
    }
  }

  m_cursorStart = editor->screenToEditor(msg->position());
  m_celOffset = gfx::Point(0, 0);
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
  if (m_celOffset != gfx::Point(0, 0)) {
    // Put the cels in the original position.
    for (size_t i=0; i<m_celList.size(); ++i) {
      Cel* cel = m_celList[i];
      const gfx::Point& celStart = m_celStarts[i];

      cel->setPosition(celStart);
    }

    // If the user didn't cancel the operation...
    if (!m_canceled) {
      ContextWriter writer(UIContext::instance(), 500);
      Transaction transaction(writer.context(), "Cel Movement", ModifyDocument);
      DocumentApi api = document->getApi(transaction);

      // And now we move the cel (or all selected range) to the new position.
      for (Cel* cel : m_celList) {
        api.setCelPosition(writer.sprite(), cel,
                           cel->x() + m_celOffset.x,
                           cel->y() + m_celOffset.y);
      }

      // Move selection if it was visible
      if (m_maskVisible)
        api.setMaskPosition(document->mask()->bounds().x + m_celOffset.x,
                            document->mask()->bounds().y + m_celOffset.y);

      transaction.commit();
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
  gfx::Point newCursorPos = editor->screenToEditor(msg->position());

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

  for (size_t i=0; i<m_celList.size(); ++i) {
    Cel* cel = m_celList[i];
    const gfx::Point& celStart = m_celStarts[i];

    cel->setPosition(celStart + m_celOffset);
  }

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
     (int)m_cursorStart.x,
     (int)m_cursorStart.y,
     (int)m_celOffset.x,
     (int)m_celOffset.y);

  return true;
}

} // namespace app
