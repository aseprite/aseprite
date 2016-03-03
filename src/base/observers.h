// Aseprite Base Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_OBSERVERS_H_INCLUDED
#define BASE_OBSERVERS_H_INCLUDED
#pragma once

#include "base/debug.h"

#include <algorithm>
#include <vector>

namespace base {

template<typename T>
class Observers {
public:
  typedef T observer_type;
  typedef std::vector<observer_type*> list_type;
  typedef typename list_type::iterator iterator;
  typedef typename list_type::const_iterator const_iterator;

  bool empty() const { return m_observers.empty(); }
  std::size_t size() const { return m_observers.size(); }

  // Adds the observer in the collection. The observer is owned by the
  // collection and will be destroyed calling the T::dispose() member
  // function.
  void addObserver(observer_type* observer) {
    ASSERT(std::find(m_observers.begin(), m_observers.end(), observer) == m_observers.end() && "You've tried to add an observer that already is in the collection");
    m_observers.push_back(observer);
  }

  // Removes the observer from the collection. After calling this
  // function you own the observer so you have to dispose it.
  void removeObserver(observer_type* observer) {
    iterator it = std::find(m_observers.begin(), m_observers.end(), observer);
    if (it != m_observers.end())
      m_observers.erase(it);
    else {
      ASSERT(false && "You've tried to remove an observer that isn't in the collection");
    }
  }

  void notifyObservers(void (observer_type::*method)()) {
    list_type copy = m_observers;
    for (iterator
           it = copy.begin(),
           end = copy.end(); it != end; ++it) {
      if (*it)
        ((*it)->*method)();
    }
  }

  template<typename A1>
  void notifyObservers(void (observer_type::*method)(A1), A1 a1) {
    list_type copy = m_observers;
    for (iterator
           it = copy.begin(),
           end = copy.end(); it != end; ++it) {
      if (*it)
        ((*it)->*method)(a1);
    }
  }

  template<typename A1, typename A2>
  void notifyObservers(void (observer_type::*method)(A1, A2), A1 a1, A2 a2) {
    list_type copy = m_observers;
    for (iterator
           it = copy.begin(),
           end = copy.end(); it != end; ++it) {
      if (*it)
        ((*it)->*method)(a1, a2);
    }
  }

  template<typename A1, typename A2, typename A3>
  void notifyObservers(void (observer_type::*method)(A1, A2, A3), A1 a1, A2 a2, A3 a3) {
    list_type copy = m_observers;
    for (iterator
           it = copy.begin(),
           end = copy.end(); it != end; ++it) {
      if (*it)
        ((*it)->*method)(a1, a2, a3);
    }
  }

private:
  list_type m_observers;
};

} // namespace base

#endif  // BASE_OBSERVERS_H_INCLUDED
