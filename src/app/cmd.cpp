/* Aseprite
 * Copyright (C) 2001-2015  David Capello
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

#include "app/cmd.h"

namespace app {

Cmd::Cmd()
#if _DEBUG
  : m_state(State::NotExecuted)
#endif
{
}

Cmd::~Cmd()
{
}

void Cmd::execute(Context* ctx)
{
  ASSERT(m_state == State::NotExecuted);

  m_ctx = ctx;

  onExecute();
  onFireNotifications();

#if _DEBUG
  m_state = State::Executed;
#endif
}

void Cmd::undo()
{
  ASSERT(m_state == State::Executed || m_state == State::Redone);

  onUndo();
  onFireNotifications();

#if _DEBUG
  m_state = State::Undone;
#endif
}

void Cmd::redo()
{
  ASSERT(m_state == State::Undone);

  onRedo();
  onFireNotifications();

#if _DEBUG
  m_state = State::Redone;
#endif
}

std::string Cmd::label() const
{
  return onLabel();
}

size_t Cmd::memSize() const
{
  return onMemSize();
}

void Cmd::onExecute()
{
  // Do nothing
}

void Cmd::onUndo()
{
  // Do nothing
}

void Cmd::onRedo()
{
  // By default onRedo() uses onExecute() implementation
  onExecute();
}

void Cmd::onFireNotifications()
{
  // Do nothing
}

std::string Cmd::onLabel() const
{
  return "";
}

size_t Cmd::onMemSize() const {
  return sizeof(*this);
}

} // namespace app
