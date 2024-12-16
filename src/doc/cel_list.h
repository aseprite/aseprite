// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CEL_LIST_H_INCLUDED
#define DOC_CEL_LIST_H_INCLUDED
#pragma once

#include <vector>

namespace doc {

class Cel;

typedef std::vector<Cel*> CelList;
typedef CelList::iterator CelIterator;
typedef CelList::const_iterator CelConstIterator;

} // namespace doc

#endif // DOC_CEL_LIST_H_INCLUDED
