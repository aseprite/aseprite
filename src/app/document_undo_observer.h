// Aseprite
// Copyright (C) 2015-2017  David Capello
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

class DocumentUndo;

  class DocumentUndoObserver {
  public:
    virtual ~DocumentUndoObserver() { }
    virtual void onAddUndoState(DocumentUndo* history) = 0;
    virtual void onDeleteUndoState(DocumentUndo* history,
                                   undo::UndoState* state) = 0;
    virtual void onAfterUndo(DocumentUndo* history) = 0;
    virtual void onAfterRedo(DocumentUndo* history) = 0;
    virtual void onClearRedo(DocumentUndo* history) = 0;
    virtual void onTotalUndoSizeChange(DocumentUndo* history) = 0;
  };

} // namespace app

#endif
