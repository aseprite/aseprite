/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "widgets/editor/moving_cel_state.h"

#include "app.h"
#include "gui/message.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "undo/undo_history.h"
#include "undoers/set_cel_position.h"
#include "widgets/editor/editor.h"
#include "widgets/statebar.h"

MovingCelState::MovingCelState(Editor* editor, Message* msg)
  : m_canceled(false)
{
  Document* document = editor->getDocument();
  Sprite* sprite = editor->getSprite();

  ASSERT(sprite->getCurrentLayer()->is_image());

  LayerImage* layer = static_cast<LayerImage*>(sprite->getCurrentLayer());

  m_cel = layer->getCel(sprite->getCurrentFrame());
  m_celStartX = m_cel->getX();
  m_celStartY = m_cel->getY();

  editor->screenToEditor(msg->mouse.x, msg->mouse.y, &m_mouseStartX, &m_mouseStartY);

  editor->captureMouse();
}

MovingCelState::~MovingCelState()
{
}

bool MovingCelState::onMouseUp(Editor* editor, Message* msg)
{
  // Here we put back the cel into its original coordinate (so we can
  // add an undoer before).
  if (m_celStartX != m_cel->getX() ||
      m_celStartY != m_cel->getY()) {
    // Hold the new cel's position
    int newX = m_cel->getX();
    int newY = m_cel->getY();

    // Put the cel in the original position.
    m_cel->setPosition(m_celStartX, m_celStartY);

    // If the user didn't cancel the operation...
    if (!m_canceled) {
       Document* document = editor->getDocument();
       undo::UndoHistory* undoHistory = document->getUndoHistory();

       // Add an undoer so we can go back to the current position (the original one).
       if (undoHistory->isEnabled()) {
	 undoHistory->setLabel("Cel Movement");
	 undoHistory->setModification(undo::ModifyDocument);
	 undoHistory->pushUndoer(new undoers::SetCelPosition(undoHistory->getObjects(), m_cel));
       }

       // And now we move the cel to the new position.
       m_cel->setPosition(newX, newY);
     }
  }

  editor->setState(editor->getDefaultState());
  editor->releaseMouse();
  return true;
}

bool MovingCelState::onMouseMove(Editor* editor, Message* msg)
{
  int newMouseX, newMouseY;
  editor->screenToEditor(msg->mouse.x, msg->mouse.y, &newMouseX, &newMouseY);

  m_cel->setPosition(m_celStartX - m_mouseStartX + newMouseX,
		     m_celStartY - m_mouseStartY + newMouseY);

  // Redraw the new cel position.
  editor->invalidate();

  // Use StandbyState implementation
  return StandbyState::onMouseMove(editor, msg);
}

bool MovingCelState::onUpdateStatusBar(Editor* editor)
{
  app_get_statusbar()->setStatusText
    (0,
     "Pos %3d %3d Offset %3d %3d",
     (int)m_cel->getX(),
     (int)m_cel->getY(),
     (int)(m_cel->getX() - m_celStartX),
     (int)(m_cel->getY() - m_celStartY));

  return true;
}
