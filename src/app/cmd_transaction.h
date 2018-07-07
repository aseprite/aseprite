// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_TRANSACTION_H_INCLUDED
#define APP_CMD_TRANSACTION_H_INCLUDED
#pragma once

#include "app/cmd_sequence.h"
#include "app/doc_range.h"
#include "app/sprite_position.h"

#include <memory>
#include <sstream>

namespace app {

  // Cmds created on each Transaction.
  // The whole DocUndo contains a list of these CmdTransaction.
  class CmdTransaction : public CmdSequence {
  public:
    CmdTransaction(const std::string& label,
      bool changeSavedState, int* savedCounter);

    void setNewDocRange(const DocRange& range);
    void commit();

    SpritePosition spritePositionBeforeExecute() const { return m_spritePositionBefore; }
    SpritePosition spritePositionAfterExecute() const { return m_spritePositionAfter; }

    std::istream* documentRangeBeforeExecute() const;
    std::istream* documentRangeAfterExecute() const;

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    std::string onLabel() const override;
    size_t onMemSize() const override;

  private:
    SpritePosition calcSpritePosition() const;
    bool isDocRangeEnabled() const;
    DocRange calcDocRange() const;

    struct Ranges {
      std::stringstream m_before;
      std::stringstream m_after;
    };

    SpritePosition m_spritePositionBefore;
    SpritePosition m_spritePositionAfter;
    std::unique_ptr<Ranges> m_ranges;
    std::string m_label;
    bool m_changeSavedState;
    int* m_savedCounter;
  };

} // namespace app

#endif
