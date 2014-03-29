// Aseprite Undo Library
// Copyright (C) 2001-2013 David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifndef UNDO_UNDO_EXCEPTION_H_INCLUDED
#define UNDO_UNDO_EXCEPTION_H_INCLUDED
#pragma once

#include <stdexcept>

namespace undo {

  class UndoException : public std::runtime_error {
  public:
    UndoException(const char* msg) throw() : std::runtime_error(msg) { }
  };

} // namespace undo

#endif  // UNDO_UNDO_EXCEPTION_H_INCLUDED
