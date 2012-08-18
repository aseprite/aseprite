/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "widgets/editor/editor_observers.h"

#include "base/bind.h"
#include "widgets/editor/editor_observer.h"

EditorObservers::EditorObservers()
{
}

void EditorObservers::addObserver(EditorObserver* observer)
{
  m_observers.addObserver(observer);
}

void EditorObservers::removeObserver(EditorObserver* observer)
{
  m_observers.removeObserver(observer);
}

void EditorObservers::notifyStateChanged(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::stateChanged, editor);
}

void EditorObservers::notifyScrollChanged(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::scrollChanged, editor);
}

void EditorObservers::notifyDocumentChanged(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::documentChanged, editor);
}
