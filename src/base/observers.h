// Aseprite Base Library
// Copyright (c) 2001-2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_OBSERVERS_H_INCLUDED
#define BASE_OBSERVERS_H_INCLUDED
#pragma once

#include <algorithm>
#include <vector>

namespace base {

template<typename T>
class Observers
{
public:
  typedef T observer_type;
  typedef std::vector<observer_type*> list_type;
  typedef typename list_type::iterator iterator;
  typedef typename list_type::const_iterator const_iterator;

  iterator begin() { return m_observers.begin(); }
  iterator end() { return m_observers.end(); }
  const_iterator begin() const { return m_observers.begin(); }
  const_iterator end() const { return m_observers.end(); }

  bool empty() const { return m_observers.empty(); }
  size_t size() const { return m_observers.size(); }

  Observers() : m_notifyingObservers(0) {
  }

  ~Observers() {
#if _DEBUG
    ASSERT(m_notifyingObservers == 0);

    bool allEmpty = true;
    for (iterator
           it = this->begin(),
           end = this->end(); it != end; ++it) {
      if (*it) {
        allEmpty = false;
      }
    }
    ASSERT(allEmpty);
#endif
  }

  // Adds the observer in the collection. The observer is owned by the
  // collection and will be destroyed calling the T::dispose() member
  // function.
  void addObserver(observer_type* observer) {
    clearNulls();

    ASSERT(std::find(m_observers.begin(), m_observers.end(), observer) == m_observers.end() && "You've tried to add an observer that already is in the collection");
    m_observers.push_back(observer);
  }

  // Removes the observer from the collection. After calling this
  // function you own the observer so you have to dispose it.
  void removeObserver(observer_type* observer) {
    clearNulls();

    iterator it = std::find(m_observers.begin(), m_observers.end(), observer);
    if (it != end())
      *it = (observer_type*)NULL;
    else {
      ASSERT(false && "You've tried to remove an observer that isn't in the collection");
    }
  }

  void notifyObservers(void (observer_type::*method)()) {
    {
      IncDec incdec(m_notifyingObservers);
      for (iterator
             it = this->begin(),
             end = this->end(); it != end; ++it) {
        if (*it)
          ((*it)->*method)();
      }
    }
    clearNulls();
  }

  template<typename A1>
  void notifyObservers(void (observer_type::*method)(A1), A1 a1) {
    {
      IncDec incdec(m_notifyingObservers);
      for (iterator
             it = this->begin(),
             end = this->end(); it != end; ++it) {
        if (*it)
          ((*it)->*method)(a1);
      }
    }
    clearNulls();
  }

  template<typename A1, typename A2>
  void notifyObservers(void (observer_type::*method)(A1, A2), A1 a1, A2 a2) {
    {
      IncDec incdec(m_notifyingObservers);
      for (iterator
             it = this->begin(),
             end = this->end(); it != end; ++it) {
        if (*it)
          ((*it)->*method)(a1, a2);
      }
    }
    clearNulls();
  }

  template<typename A1, typename A2, typename A3>
  void notifyObservers(void (observer_type::*method)(A1, A2, A3), A1 a1, A2 a2, A3 a3) {
    {
      IncDec incdec(m_notifyingObservers);
      for (iterator
             it = this->begin(),
             end = this->end(); it != end; ++it) {
        if (*it)
          ((*it)->*method)(a1, a2, a3);
      }
    }
    clearNulls();
  }

private:
  void clearNulls() {
    if (m_notifyingObservers > 0)
      return;

    iterator it;
    while ((it = std::find(
          m_observers.begin(),
          m_observers.end(), (observer_type*)NULL)) != m_observers.end()) {
      m_observers.erase(it);
    }
  }

  struct IncDec {
    int& m_value;
    IncDec(int& value) : m_value(value) { ++m_value; }
    ~IncDec() { --m_value; }
  };

  list_type m_observers;
  int m_notifyingObservers;
};

} // namespace base

#endif  // BASE_OBSERVERS_H_INCLUDED
