// Aseprite Undo2 Library
// Copyright (C) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UNDO2_UNDO_COMMAND_H_INCLUDED
#define UNDO2_UNDO_COMMAND_H_INCLUDED
#pragma once

namespace undo2 {

  class UndoCommand {
  public:
    virtual ~UndoCommand() { }
    virtual void undo() = 0;
    virtual void redo() = 0;
  };

} // namespace undo2

#endif  // UNDO2_UNDO_COMMAND_H_INCLUDED
