// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_PREF_OPTION_H_INCLUDED
#define APP_PREF_OPTION_H_INCLUDED
#pragma once

#include "base/signal.h"

namespace app {

  template<typename T>
  class Option {
  public:
    Option(const char* section, const char* id, const T& defaultValue = T())
      : m_section(section)
      , m_id(id)
      , m_default(defaultValue)
      , m_value(defaultValue)
      , m_dirty(false) {
    }

    Option(const Option& opt)
      : m_section(opt.m_section)
      , m_id(opt.m_id)
      , m_default(opt.m_default)
      , m_value(opt.m_value)
      , m_dirty(false) {
    }

    Option& operator=(const Option& opt) {
      operator()(opt.m_value());
      return *this;
    }

    const char* section() const { return m_section; }
    const char* id() const { return m_id; }
    const T& defaultValue() const { return m_default; }

    bool isDirty() const { return m_dirty; }
    void forceDirtyFlag() { m_dirty = true; }
    void cleanDirtyFlag() { m_dirty = false; }

    const T& operator()() const {
      return m_value;
    }

    const T& operator()(const T& newValue) {
      if (m_value == newValue)
        return m_value;

      BeforeChange(*this, newValue);

      T oldValue = m_value;
      m_value = newValue;
      m_dirty = true;

      AfterChange(*this, oldValue);
      return m_value;
    }

    Signal2<void, Option&, const T&> BeforeChange;
    Signal2<void, Option&, const T&> AfterChange;

  private:
    const char* m_section;
    const char* m_id;
    T m_default;
    T m_value;
    bool m_dirty;

    Option();
  };

} // namespace app

#endif
