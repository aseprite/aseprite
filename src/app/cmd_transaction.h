// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
