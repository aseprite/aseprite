// Aseprite
// Copyright (C) 2021-2022  Igara Studio S.A.
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_DOCUMENT_UNDO_OBSERVER_H_INCLUDED
#define APP_DOCUMENT_UNDO_OBSERVER_H_INCLUDED
#pragma once

namespace undo {
class UndoState;
}

namespace app {

class DocUndo;

class DocUndoObserver {
public:
  virtual ~DocUndoObserver() {}
  virtual void onAddUndoState(DocUndo* history) {}
  virtual void onDeleteUndoState(DocUndo* history, undo::UndoState* state) {}
  virtual void onCurrentUndoStateChange(DocUndo* history) {}
  virtual void onClearRedo(DocUndo* history) {}
  virtual void onTotalUndoSizeChange(DocUndo* history) {}
  virtual void onNewSavedState(DocUndo* history) {}
};

} // namespace app

#endif
