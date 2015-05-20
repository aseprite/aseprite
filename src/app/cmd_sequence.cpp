// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd_sequence.h"

namespace app {

CmdSequence::CmdSequence()
{
}

CmdSequence::~CmdSequence()
{
  for (Cmd* cmd : m_cmds)
    delete cmd;
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
