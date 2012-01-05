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

#include "widgets/editor/editor_listeners.h"

#include "base/bind.h"
#include "widgets/editor/editor_listener.h"

EditorListeners::EditorListeners()
{
}

void EditorListeners::addListener(EditorListener* listener)
{
  m_listeners.addListener(listener);
}

void EditorListeners::removeListener(EditorListener* listener)
{
  m_listeners.removeListener(listener);
}

void EditorListeners::notifyStateChanged(Editor* editor)
{
  m_listeners.notify(&EditorListener::stateChanged, editor);
}

void EditorListeners::notifyScrollChanged(Editor* editor)
{
  m_listeners.notify(&EditorListener::scrollChanged, editor);
}

void EditorListeners::notifyDocumentChanged(Editor* editor)
{
  m_listeners.notify(&EditorListener::documentChanged, editor);
}
