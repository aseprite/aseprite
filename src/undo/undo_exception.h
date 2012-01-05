// ASEPRITE Undo Library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UNDO_UNDO_EXCEPTION_H_INCLUDED
#define UNDO_UNDO_EXCEPTION_H_INCLUDED

#include "base/exception.h"

namespace undo {

class UndoException : public base::Exception
{
public:
  UndoException(const char* msg) throw() : base::Exception(msg) { }
};

} // namespace undo

#endif  // UNDO_UNDO_EXCEPTION_H_INCLUDED
