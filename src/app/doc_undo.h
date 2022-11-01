// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOCUMENT_UNDO_H_INCLUDED
#define APP_DOCUMENT_UNDO_H_INCLUDED
#pragma once

#include "app/doc_range.h"
#include "app/sprite_position.h"
#include "base/disable_copying.h"
#include "obs/observable.h"
#include "undo/undo_history.h"

#include <iosfwd>
#include <string>

namespace app {
  using namespace doc;

  class Cmd;
  class CmdTransaction;
  class Context;
  class DocUndoObserver;

  class DocUndo : public obs::observable<DocUndoObserver>,
                  public undo::UndoHistoryDelegate {
  public:
    DocUndo();

    size_t totalUndoSize() const { return m_totalUndoSize; }

    void setContext(Context* ctx);

    void add(CmdTransaction* cmd);

    bool canUndo() const;
    bool canRedo() const;
    void undo();
    void redo();

    void clearRedo();

    bool isInSavedState() const;
    bool isSavedStateIsLost() const { return m_savedStateIsLost; }
    void markSavedState();
    void impossibleToBackToSavedState();
    const undo::UndoState* savedState() const { return m_savedState; }

    std::string nextUndoLabel() const;
    std::string nextRedoLabel() const;

    SpritePosition nextUndoSpritePosition() const;
    SpritePosition nextRedoSpritePosition() const;
    std::istream* nextUndoDocRange() const;
    std::istream* nextRedoDocRange() const;

    Cmd* lastExecutedCmd() const;

    const undo::UndoState* firstState() const   { return m_undoHistory.firstState(); }
    const undo::UndoState* lastState() const    { return m_undoHistory.lastState(); }
    const undo::UndoState* currentState() const { return m_undoHistory.currentState(); }

    void moveToState(const undo::UndoState* state);

  private:
    const undo::UndoState* nextUndo() const;
    const undo::UndoState* nextRedo() const;

    // undo::UndoHistoryDelegate impl
    void onDeleteUndoState(undo::UndoState* state) override;

    undo::UndoHistory m_undoHistory;
    const undo::UndoState* m_savedState = nullptr;
    Context* m_ctx = nullptr;
    size_t m_totalUndoSize = 0;

    // True if the saved state was invalidated/corrupted/lost in some
    // way. E.g. If the save process fails.
    bool m_savedStateIsLost = false;

    DISABLE_COPYING(DocUndo);
  };

} // namespace app

#endif
