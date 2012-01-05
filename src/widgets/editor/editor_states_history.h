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

#ifndef WIDGETS_EDITOR_STATES_HISTORY_H_INCLUDED
#define WIDGETS_EDITOR_STATES_HISTORY_H_INCLUDED

#include "widgets/editor/editor_state.h"

#include <vector>

// Stack of states in the editor. The top state is the current state
// of the editor.
class EditorStatesHistory
{
public:
  EditorStatesHistory();
  ~EditorStatesHistory();

  // Gets the current state.
  EditorStatePtr top();

  // Adds a new state in the history.
  void push(const EditorStatePtr& state);

  // Removes a state from the history (you should keep a reference of
  // the state with top() if you want to keep it in memory).
  void pop();

  // Deletes all the history.
  void clear();

private:
  // Stack of states (the back of the vector is the top of the stack).
  std::vector<EditorStatePtr> m_states;
};

#endif
