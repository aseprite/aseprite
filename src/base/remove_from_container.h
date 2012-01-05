// ASE base library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_REMOVE_FROM_CONTAINER_H_INCLUDED
#define BASE_REMOVE_FROM_CONTAINER_H_INCLUDED

namespace base {

// Removes the first ocurrence of the specified element from the STL
// container.
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
