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

#include "config.h"

#include "widgets/editor/editor_states_history.h"

EditorStatesHistory::EditorStatesHistory()
{
}

EditorStatesHistory::~EditorStatesHistory()
{
  clear();
}

EditorStatePtr EditorStatesHistory::top()
{
  return (!m_states.empty() ? m_states.back(): EditorStatePtr(NULL));
}

void EditorStatesHistory::push(const EditorStatePtr& state)
{
  ASSERT(state != NULL);
  m_states.push_back(state);
}

void EditorStatesHistory::pop()
{
  ASSERT(!m_states.empty());
  m_states.pop_back();
}

void EditorStatesHistory::clear()
{
  // Free shared pointers in reverse order
  std::vector<EditorStatePtr>::reverse_iterator it = m_states.rbegin();
  std::vector<EditorStatePtr>::reverse_iterator end = m_states.rend();
  for (; it != end; ++it)
    (*it).reset();

  m_states.clear();
}
