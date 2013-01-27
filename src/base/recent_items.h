// ASEPRITE base library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef BASE_RECENT_ITEMS_H_INCLUDED
#define BASE_RECENT_ITEMS_H_INCLUDED

#include <list>
#include <algorithm>

namespace base {

template<typename T>
class RecentItems {
public:
  typedef std::list<T> Items;
  typedef typename Items::iterator iterator;
  typedef typename Items::const_iterator const_iterator;

  RecentItems(size_t limit) : m_limit(limit) { }

  const_iterator begin() { return m_items.begin(); }
  const_iterator end() { return m_items.end(); }

  bool empty() const { return m_items.empty(); }
  size_t size() const { return m_items.size(); }
  size_t limit() const { return m_limit; }

  void addItem(const T& item) {
    iterator it = std::find(m_items.begin(), m_items.end(), item);

    // If the item already exist in the list...
    if (it != m_items.end()) {
      // Move it to the first position
      m_items.erase(it);
      m_items.insert(m_items.begin(), item);
      return;
    }

    // Does the list is full?
    if (m_items.size() == m_limit) {
      // Remove the last entry
      m_items.erase(--m_items.end());
    }

    m_items.insert(m_items.begin(), item);
  }

  void removeItem(const T& item) {
    iterator it = std::find(m_items.begin(), m_items.end(), item);
    if (it != m_items.end())
      m_items.erase(it);
  }

private:
  Items m_items;
  size_t m_limit;
};

} // namespace base

#endif
