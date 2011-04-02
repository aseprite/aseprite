/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#ifndef LISTENERS_H_INCLUDED
#define LISTENERS_H_INCLUDED

#include <algorithm>
#include <vector>

template<typename T>
class Listeners
{
public:
  typedef T listener_type;
  typedef std::vector<listener_type*> list_type;
  typedef typename list_type::iterator iterator;
  typedef typename list_type::const_iterator const_iterator;

  iterator begin() { return m_listeners.begin(); }
  iterator end() { return m_listeners.end(); }
  const_iterator begin() const { return m_listeners.begin(); }
  const_iterator end() const { return m_listeners.end(); }

  bool empty() const { return m_listeners.empty(); }
  size_t size() const { return m_listeners.size(); }

  Listeners() { }

  ~Listeners()
  {
    disposeAllListeners();
  }

  // Adds the listener in the collection. The listener is owned by the
  // collection and will be destroyed calling the T::dispose() member
  // function.
  void addListener(listener_type* listener)
  {
    m_listeners.push_back(listener);
  }

  // Removes the listener from the collection. After calling this
  // function you own the listener so you have to dispose it.
  void removeListener(listener_type* listener)
  {
    iterator it = std::find(m_listeners.begin(), m_listeners.end(), listener);
    if (it != end())
      m_listeners.erase(it);
  }

  // Disposes all listeners. It's called automatically in the
  // Listeners dtor.
  void disposeAllListeners()
  {
    while (!empty())
      (*begin())->dispose();

    m_listeners.clear();
  }

  void notify(void (listener_type::*method)())
  {
    for (iterator
	   it = this->begin(), 
	   end = this->end(); it != end; ++it) {
      ((*it)->*method)();
    }
  }

  template<typename Arg1>
  void notify(void (listener_type::*method)(Arg1), Arg1 arg1)
  {
    for (iterator
	   it = this->begin(), 
	   end = this->end(); it != end; ++it) {
      ((*it)->*method)(arg1);
    }
  }

private:
  list_type m_listeners;
};

#endif	// LISTENERS_H_INCLUDED
