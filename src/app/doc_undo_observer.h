// Aseprite
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
    virtual ~DocUndoObserver() { }
    virtual void onAddUndoState(DocUndo* history) = 0;
    virtual void onDeleteUndoState(DocUndo* history,
                                   undo::UndoState* state) = 0;
    virtual void onCurrentUndoStateChange(DocUndo* history) = 0;
    virtual void onClearRedo(DocUndo* history) = 0;
    virtual void onTotalUndoSizeChange(DocUndo* history) = 0;
  };

} // namespace app

#endif
