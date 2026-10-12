// Aseprite
// Copyright (C) 2023-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/sequence.h"

#include "app/context.h"

namespace app { namespace cmd {

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

void CmdSequence::addAndExecute(Context* ctx, Cmd* cmd)
{
  // First we add the cmd to the list of commands (m_cmds). In this way
  add(cmd);

  // Index where the cmd was added just in case to remove it if we
  // catch an exception.
  const int i = m_cmds.size() - 1;

  try {
    // After we've added the cmd to the cmds list, we can execute
    // it. As this execution can generate signals/notifications (like
    // onActiveSiteChange), those who are listening to those
    // notifications can add and execute more cmds (and we have to add
    // all of them in order, that's why the cmd was added in m_cmds in
    // the first place).
    cmd->execute(ctx);
  }
  catch (...) {
    m_cmds.erase(m_cmds.begin() + i);
    throw;
  }
}

void CmdSequence::onExecute(Context* ctx)
{
  for (auto* cmd : m_cmds)
    cmd->execute(ctx);
}

void CmdSequence::onUndo(Context* ctx)
{
  for (auto it = m_cmds.rbegin(), end = m_cmds.rend(); it != end; ++it)
    (*it)->undo(ctx);
}

void CmdSequence::onRedo(Context* ctx)
{
  for (auto* cmd : m_cmds)
    cmd->redo(ctx);
}

size_t CmdSequence::onMemSize() const
{
  size_t size = sizeof(*this);
  for (const auto* cmd : m_cmds)
    size += cmd->memSize();
  return size;
}

void CmdSequence::executeAndAdd(Context* ctx, Cmd* cmd)
{
  addAndExecute(ctx, cmd);
}

bool CmdSequence::onIsSerializable() const
{
  for (Cmd* cmd : m_cmds) {
    if (cmd->isSerializable())
      return true;
  }
  return false;
}

void CmdSequence::onSerialize(CmdSerial& s)
{
  bool first = true;
  s.seqBegin();
  if (s.encoding()) {
    for (Cmd* cmd : m_cmds) {
      if (!cmd->isSerializable())
        continue;

      if (first)
        first = false;
      else
        s.seqSeparator();
      cmd->serialize(s);
    }
  }
  else {
    while (Cmd* cmd = Cmd::Decode(s)) {
      m_cmds.push_back(cmd);
      s.seqSeparator();
    }
  }
  s.seqEnd();
}

}} // namespace app::cmd
