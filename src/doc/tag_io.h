// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_TAG_IO_H_INCLUDED
#define DOC_TAG_IO_H_INCLUDED
#pragma once

#include <iosfwd>

namespace doc {

  class Tag;

  void write_tag(std::ostream& os, const Tag* tag);
  Tag* read_tag(std::istream& is, bool setId = true);

} // namespace doc

#endif
