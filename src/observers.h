/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef OBSERVERS_H_INCLUDED
#define OBSERVERS_H_INCLUDED

#include <algorithm>
#include <vector>

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

  Observers() { }

  ~Observers() {
    disposeAllObservers();
  }

  // Adds the observer in the collection. The observer is owned by the
  // collection and will be destroyed calling the T::dispose() member
  // function.
  void addObserver(observer_type* observer) {
    m_observers.push_back(observer);
  }

  // Removes the observer from the collection. After calling this
  // function you own the observer so you have to dispose it.
  void removeObserver(observer_type* observer) {
    iterator it = std::find(m_observers.begin(), m_observers.end(), observer);
    if (it != end())
      m_observers.erase(it);
  }

  // Disposes all observers. It's called automatically in the
  // Observers dtor.
  void disposeAllObservers() {
    while (!empty())
      (*begin())->dispose();

    m_observers.clear();
  }

  void notifyObservers(void (observer_type::*method)()) {
    for (iterator
           it = this->begin(),
           end = this->end(); it != end; ++it) {
      ((*it)->*method)();
    }
  }

  template<typename A1>
  void notifyObservers(void (observer_type::*method)(A1), A1 a1) {
    for (iterator
           it = this->begin(),
           end = this->end(); it != end; ++it) {
      ((*it)->*method)(a1);
    }
  }

  template<typename A1, typename A2>
  void notifyObservers(void (observer_type::*method)(A1, A2), A1 a1, A2 a2) {
    for (iterator
           it = this->begin(),
           end = this->end(); it != end; ++it) {
      ((*it)->*method)(a1, a2);
    }
  }

  template<typename A1, typename A2, typename A3>
  void notifyObservers(void (observer_type::*method)(A1, A2, A3), A1 a1, A2 a2, A3 a3) {
    for (iterator
           it = this->begin(),
           end = this->end(); it != end; ++it) {
      ((*it)->*method)(a1, a2, a3);
    }
  }

private:
  list_type m_observers;
};

#endif  // OBSERVERS_H_INCLUDED
