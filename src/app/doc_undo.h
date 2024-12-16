// Aseprite
// Copyright (C) 2022-2023  Igara Studio S.A.
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
#include "base/exception.h"
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

// Exception thrown when we want to modify the sprite (add new
// app::Cmd objects) when we are undoing/redoing/moving throw the
// undo history.
class CannotModifyWhenUndoingException : public base::Exception {
public:
  CannotModifyWhenUndoingException() throw()
    : base::Exception("Cannot modify the sprite when we are undoing/redoing an action.")
  {
  }
};

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

  // Returns true we are in the UndoState that matches the sprite
  // version on the disk (or we are in a similar state that doesn't
  // modify that same state, e.g. if the current state modifies the
  // selection but not the pixels, we are in a similar state)
  bool isInSavedStateOrSimilar() const;

  // Returns true if the saved state was lost, e.g. because we
  // deleted the redo history and the saved state was there.
  bool isSavedStateIsLost() const { return m_savedStateIsLost; }

  // Marks current UndoState as the one that matches the sprite on
  // the disk (this is used after saving the file).
  void markSavedState();

  // Indicates that now it's impossible to back to the version of
  // the sprite that matches the saved version. This can be because
  // the save process fails or because we deleted the redo history
  // where the saved state was available.
  void impossibleToBackToSavedState();

  // Returns the position in the undo history where this sprite was
  // saved, if this is nullptr, it means that the initial state is
  // the saved state (if m_savedStateIsLost is false) or it means
  // that there is no saved state (ifm_savedStateIsLost is true)
  const undo::UndoState* savedState() const { return m_savedState; }

  std::string nextUndoLabel() const;
  std::string nextRedoLabel() const;

  SpritePosition nextUndoSpritePosition() const;
  SpritePosition nextRedoSpritePosition() const;
  std::istream* nextUndoDocRange() const;
  std::istream* nextRedoDocRange() const;

  Cmd* lastExecutedCmd() const;

  const undo::UndoState* firstState() const { return m_undoHistory.firstState(); }
  const undo::UndoState* lastState() const { return m_undoHistory.lastState(); }
  const undo::UndoState* currentState() const { return m_undoHistory.currentState(); }

  bool isUndoing() const { return m_undoing; }

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

  // True when we are undoing/redoing. Used to avoid adding new undo
  // information when we are moving through the undo history.
  bool m_undoing = false;

  // True if the saved state was invalidated/corrupted/lost in some
  // way. E.g. If the save process fails.
  bool m_savedStateIsLost = false;

  DISABLE_COPYING(DocUndo);
};

} // namespace app

#endif
