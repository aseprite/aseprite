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

#ifndef APP_CMD_TRANSACTION_H_INCLUDED
#define APP_CMD_TRANSACTION_H_INCLUDED
#pragma once

#include "app/cmd_sequence.h"

namespace app {

  // Cmds created on each Transaction.
  // The whole DocumentUndo contains a list of these CmdTransaction.
  class CmdTransaction : public CmdSequence {
  public:
    CmdTransaction(const std::string& label,
      bool changeSavedState, int* savedCounter);

    void commit();

    doc::SpritePosition spritePositionBeforeExecute() const { return m_spritePositionBefore; }
    doc::SpritePosition spritePositionAfterExecute() const { return m_spritePositionAfter; }

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    std::string onLabel() const override;

  private:
    doc::SpritePosition calcSpritePosition();

    doc::SpritePosition m_spritePositionBefore;
    doc::SpritePosition m_spritePositionAfter;
    std::string m_label;
    bool m_changeSavedState;
    int* m_savedCounter;
  };

} // namespace app

#endif
