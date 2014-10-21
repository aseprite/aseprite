// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_LAYER_LIST_H_INCLUDED
#define DOC_LAYER_LIST_H_INCLUDED
#pragma once

#include <list>

namespace doc {

  class Layer;

  typedef std::list<Layer*> LayerList;
  typedef std::list<Layer*>::iterator LayerIterator;
  typedef std::list<Layer*>::const_iterator LayerConstIterator;

} // namespace doc

#endif  // DOC_LAYER_LIST_H_INCLUDED
