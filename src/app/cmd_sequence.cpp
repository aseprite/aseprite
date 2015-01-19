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

#include "app/cmd_sequence.h"

namespace app {

CmdSequence::CmdSequence()
{
}

void CmdSequence::add(Cmd* cmd)
{
  m_cmds.push_back(cmd);
}

void CmdSequence::onExecute()
{
  for (auto it = m_cmds.begin(), end=m_cmds.end(); it!=end; ++it)
    (*it)->execute(context());
}

void CmdSequence::onUndo()
{
  for (auto it = m_cmds.rbegin(), end=m_cmds.rend(); it!=end; ++it)
    (*it)->undo();
}

void CmdSequence::onRedo()
{
  for (auto it = m_cmds.begin(), end=m_cmds.end(); it!=end; ++it)
    (*it)->redo();
}

size_t CmdSequence::onMemSize() const
{
  size_t size = sizeof(*this);

  for (auto it = m_cmds.begin(), end=m_cmds.end(); it!=end; ++it)
    size += (*it)->memSize();

  return size;
}

void CmdSequence::executeAndAdd(Cmd* cmd)
{
  cmd->execute(context());
  add(cmd);
}

} // namespace app
