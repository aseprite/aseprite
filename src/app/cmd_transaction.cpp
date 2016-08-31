// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd_transaction.h"

#include "app/context.h"
#include "doc/site.h"

namespace app {

CmdTransaction::CmdTransaction(const std::string& label,
  bool changeSavedState, int* savedCounter)
  : m_label(label)
  , m_changeSavedState(changeSavedState)
  , m_savedCounter(savedCounter)
{
}

void CmdTransaction::commit()
{
  m_spritePositionAfter = calcSpritePosition();
}

void CmdTransaction::onExecute()
{
  CmdSequence::onExecute();

  // The execution of CmdTransaction is called by Transaction at the
  // very beginning, just to save the current sprite position.
  m_spritePositionBefore = calcSpritePosition();

  if (m_changeSavedState)
    ++(*m_savedCounter);
}

void CmdTransaction::onUndo()
{
  CmdSequence::onUndo();

  if (m_changeSavedState)
    --(*m_savedCounter);
}

void CmdTransaction::onRedo()
{
  CmdSequence::onRedo();

  if (m_changeSavedState)
    ++(*m_savedCounter);
}

std::string CmdTransaction::onLabel() const
{
  return m_label;
}

doc::SpritePosition CmdTransaction::calcSpritePosition()
{
  doc::Site site = context()->activeSite();
  return doc::SpritePosition(site.layer(), site.frame());
}

} // namespace app
