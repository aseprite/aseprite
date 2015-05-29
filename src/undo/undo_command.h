// Aseprite Undo Library
// Copyright (C) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UNDO_UNDO_COMMAND_H_INCLUDED
#define UNDO_UNDO_COMMAND_H_INCLUDED
#pragma once

namespace undo {

  class UndoCommand {
  public:
    virtual ~UndoCommand() { }
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual void dispose() = 0;
  };

} // namespace undo

#endif  // UNDO_UNDO_COMMAND_H_INCLUDED
