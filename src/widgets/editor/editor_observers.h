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

#ifndef WIDGETS_EDITOR_OBSERVERS_H_INCLUDED
#define WIDGETS_EDITOR_OBSERVERS_H_INCLUDED

#include "observers.h"
#include "widgets/editor/editor_observer.h"

class Editor;

class EditorObservers
{
public:
  EditorObservers();

  void addObserver(EditorObserver* observer);
  void removeObserver(EditorObserver* observer);

  void notifyStateChanged(Editor* editor);
  void notifyScrollChanged(Editor* editor);
  void notifyDocumentChanged(Editor* editor);

private:
  Observers<EditorObserver> m_observers;
};

#endif  // WIDGETS_EDITOR_OBSERVERS_H_INCLUDED
