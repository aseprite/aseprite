// ASEPRITE Undo Library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UNDO_UNDO_CONFIG_PROVIDER_H_INCLUDED
#define UNDO_UNDO_CONFIG_PROVIDER_H_INCLUDED

namespace undo {

class UndoConfigProvider
{
public:
  virtual ~UndoConfigProvider() { }

  // Returns the limit of undo history in bytes.
  virtual size_t getUndoSizeLimit() = 0;
};

} // namespace undo

#endif  // UNDO_UNDO_CONFIG_PROVIDER_H_INCLUDED
