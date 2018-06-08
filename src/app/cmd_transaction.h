// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_TRANSACTION_H_INCLUDED
#define APP_CMD_TRANSACTION_H_INCLUDED
#pragma once

#include "app/cmd_sequence.h"
#include "app/document_range.h"

#include <memory>
#include <sstream>

namespace app {

  // Cmds created on each Transaction.
  // The whole DocumentUndo contains a list of these CmdTransaction.
  class CmdTransaction : public CmdSequence {
  public:
    CmdTransaction(const std::string& label,
      bool changeSavedState, int* savedCounter);

    void setNewDocumentRange(const DocumentRange& range);
    void commit();

    doc::SpritePosition spritePositionBeforeExecute() const { return m_spritePositionBefore; }
    doc::SpritePosition spritePositionAfterExecute() const { return m_spritePositionAfter; }

    std::istream* documentRangeBeforeExecute() const;
    std::istream* documentRangeAfterExecute() const;

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    std::string onLabel() const override;
    size_t onMemSize() const override;

  private:
    doc::SpritePosition calcSpritePosition() const;
    bool isDocumentRangeEnabled() const;
    DocumentRange calcDocumentRange() const;

    struct Ranges {
      std::stringstream m_before;
      std::stringstream m_after;
    };

    doc::SpritePosition m_spritePositionBefore;
    doc::SpritePosition m_spritePositionAfter;
    std::unique_ptr<Ranges> m_ranges;
    std::string m_label;
    bool m_changeSavedState;
    int* m_savedCounter;
  };

} // namespace app

#endif
