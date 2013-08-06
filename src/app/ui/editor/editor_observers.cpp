/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/editor_observers.h"

#include "app/ui/editor/editor_observer.h"
#include "base/bind.h"

namespace app {
  
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
  m_observers.notifyObservers(&EditorObserver::onStateChanged, editor);
}

void EditorObservers::notifyScrollChanged(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::onScrollChanged, editor);
}

void EditorObservers::notifyFrameChanged(Editor* editor)
{
  m_observers.notifyObservers(&EditorObserver::onFrameChanged, editor);
}

} // namespace app
