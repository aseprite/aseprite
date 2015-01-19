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

#include "app/cmd_transaction.h"

#include "app/context.h"
#include "app/document_location.h"

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
  app::DocumentLocation loc = context()->activeLocation();
  return doc::SpritePosition(loc.layerIndex(), loc.frame());
}

} // namespace app
