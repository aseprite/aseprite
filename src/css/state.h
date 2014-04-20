// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CSS_STATE_H_INCLUDED
#define CSS_STATE_H_INCLUDED
#pragma once

#include <string>
#include <vector>

namespace css {

  class State {
  public:
    State() { }
    State(const std::string& name) : m_name(name) { }

    const std::string& name() const { return m_name; }

  private:
    std::string m_name;
  };

  class States {
  public:
    typedef std::vector<const State*> List;
    typedef List::iterator iterator;
    typedef List::const_iterator const_iterator;
    typedef List::reverse_iterator reverse_iterator;
    typedef List::const_reverse_iterator const_reverse_iterator;

    States() { }
    States(const State& state) {
      operator+=(state);
    }

    iterator begin() { return m_list.begin(); }
    iterator end() { return m_list.end(); }
    const_iterator begin() const { return m_list.begin(); }
    const_iterator end() const { return m_list.end(); }
    reverse_iterator rbegin() { return m_list.rbegin(); }
    reverse_iterator rend() { return m_list.rend(); }
    const_reverse_iterator rbegin() const { return m_list.rbegin(); }
    const_reverse_iterator rend() const { return m_list.rend(); }

    States& operator+=(const State& other) {
      m_list.push_back(&other);
      return *this;
    }

    States& operator+=(const States& others) {
      for (const_iterator it=others.begin(), end=others.end(); it != end; ++it)
        operator+=(*(*it));
      return *this;
    }

    bool operator<(const States& other) const {
      return m_list < other.m_list;
    }

  private:
    List m_list;
  };

  inline States operator+(const State& a, const State& b) {
    States states;
    states += a;
    states += b;
    return states;
  }

} // namespace css

#endif
