// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SELECTED_OBJECTS_H_INCLUDED
#define DOC_SELECTED_OBJECTS_H_INCLUDED
#pragma once

#include "base/debug.h"
#include "doc/object.h"
#include "doc/object_id.h"

#include <set>

namespace doc {

  // A set of selected objects (e.g. select set of slices)
  class SelectedObjects {
  public:
    typedef std::set<ObjectId> Set;
    typedef Set::iterator iterator;
    typedef Set::const_iterator const_iterator;

    template<typename T>
    class IteratableAs {
    public:
      class IteratorAs {
      public:
        typedef T* value_type;
        typedef std::ptrdiff_t difference_type;
        typedef T** pointer;
        typedef T*& reference;
        typedef std::forward_iterator_tag iterator_category;
        IteratorAs(const const_iterator& it) : m_it(it) { }
        bool operator==(const IteratorAs& other) const {
          return m_it == other.m_it;
        }
        bool operator!=(const IteratorAs& other) const {
          return !operator==(other);
        }
        T* operator*() const {
          return doc::get<T>(*m_it);
        }
        IteratorAs& operator++() {
          ++m_it;
          return *this;
        }
      private:
        const_iterator m_it;
      };

      typedef IteratorAs iterator;

      iterator begin() const { return m_begin; }
      iterator end() const { return m_end; }

      IteratableAs(const SelectedObjects& objs)
        : m_begin(objs.begin())
        , m_end(objs.end()) { }
    private:
      IteratorAs m_begin, m_end;
    };

    iterator begin() { return m_set.begin(); }
    iterator end() { return m_set.end(); }
    const_iterator begin() const { return m_set.begin(); }
    const_iterator end() const { return m_set.end(); }

    bool empty() const { return m_set.empty(); }
    size_t size() const { return m_set.size(); }

    void clear() {
      m_set.clear();
    }

    void insert(const ObjectId id) {
      m_set.insert(id);
    }

    bool contains(const ObjectId id) const {
      return (m_set.find(id) != end());
    }

    void erase(const ObjectId id) {
      auto it = m_set.find(id);
      if (it != end())
        m_set.erase(it);
    }

    template<typename T>
    T* frontAs() const {
      ASSERT(!m_set.empty());
      return doc::get<T>(*m_set.begin());
    }

    template<typename T>
    IteratableAs<T> iterateAs() const { return IteratableAs<T>(*this); }

  private:
    Set m_set;
  };

} // namespace doc

#endif  // DOC_SELECTED_OBJECTS_H_INCLUDED
