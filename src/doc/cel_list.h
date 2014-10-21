// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_CEL_LIST_H_INCLUDED
#define DOC_CEL_LIST_H_INCLUDED
#pragma once

#include <list>

namespace doc {

  class Cel;

  typedef std::list<Cel*> CelList;
  typedef std::list<Cel*>::iterator CelIterator;
  typedef std::list<Cel*>::const_iterator CelConstIterator;

} // namespace doc

#endif  // DOC_CEL_LIST_H_INCLUDED
