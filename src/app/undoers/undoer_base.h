// ASEPRITE Undo Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef APP_UNDOERS_UNDOER_BASE_H_INCLUDED
#define APP_UNDOERS_UNDOER_BASE_H_INCLUDED

#include "base/compiler_specific.h"
#include "undo/undoer.h"

namespace app {
  namespace undoers {

    // Helper class to make new Undoers, derive from here and implement
    // revert(), getMemSize(), and dispose() methods only.
    class UndoerBase : public undo::Undoer {
    public:
      undo::Modification getModification() const OVERRIDE { return undo::DoesntModifyDocument; }
      bool isOpenGroup() const OVERRIDE { return false; }
      bool isCloseGroup() const OVERRIDE { return false; }
    };

  } // namespace undoers
} // namespace app

#endif  // UNDOERS_UNDOER_BASE_H_INCLUDED
