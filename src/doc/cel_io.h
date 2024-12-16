// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CEL_IO_H_INCLUDED
#define DOC_CEL_IO_H_INCLUDED
#pragma once

#include <iosfwd>

namespace doc {

class Cel;
class SubObjectsIO;

void write_cel(std::ostream& os, const Cel* cel);
Cel* read_cel(std::istream& is, SubObjectsIO* subObjects, bool setId = true);

} // namespace doc

#endif
