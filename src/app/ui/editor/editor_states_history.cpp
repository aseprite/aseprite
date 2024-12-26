// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/editor/editor_states_history.h"

namespace app {

EditorStatesHistory::EditorStatesHistory()
{
}

EditorStatesHistory::~EditorStatesHistory()
{
  clear();
}

EditorStatePtr EditorStatesHistory::top()
{
  return (!m_states.empty() ? m_states.back() : EditorStatePtr(NULL));
}

void EditorStatesHistory::push(const EditorStatePtr& state)
{
  ASSERT(state);
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

} // namespace app
