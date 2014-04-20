// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_REMOVE_FROM_CONTAINER_H_INCLUDED
#define BASE_REMOVE_FROM_CONTAINER_H_INCLUDED
#pragma once

namespace base {

// Removes all ocurrences of the specified element from the STL container.
template<typename ContainerType>
void remove_from_container(ContainerType& container,
                           typename ContainerType::const_reference element)
{
  for (typename ContainerType::iterator
         it = container.begin(),
         end = container.end(); it != end; ) {
    if (*it == element) {
      it = container.erase(it);
      end = container.end();
    }
    else
      ++it;
  }
}

}

#endif
