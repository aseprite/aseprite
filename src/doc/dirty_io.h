// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_DIRTY_IO_H_INCLUDED
#define DOC_DIRTY_IO_H_INCLUDED
#pragma once

#include <iosfwd>

namespace doc {

  class Dirty;

  void write_dirty(std::ostream& os, Dirty* dirty);
  Dirty* read_dirty(std::istream& is);

} // namespace doc

#endif
