// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_PREF_OPTION_H_INCLUDED
#define APP_PREF_OPTION_H_INCLUDED
#pragma once

#include "base/signal.h"
#include <string>

namespace app {

  class Section {
  public:
    Section(const std::string& name) : m_name(name) { }
    const char* name() const { return m_name.c_str(); }

    base::Signal0<void> BeforeChange;
    base::Signal0<void> AfterChange;

  private:
    std::string m_name;
  };

  template<typename T>
  class Option {
  public:
    Option(Section* section, const char* id, const T& defaultValue = T())
      : m_section(section)
      , m_id(id)
      , m_default(defaultValue)
      , m_value(defaultValue)
      , m_dirty(false) {
    }

    const T& operator()() const {
      return m_value;
    }

    const T& operator()(const T& newValue) {
      setValue(newValue);
      return m_value;
    }

    // operator=() changes the value and the default value of the
    // option.  E.g. it's used when we copy two complete Sections,
    // from default settings to user settings, or viceversa (the
    // default value is always involved).
    Option& operator=(const Option& opt) {
      setValueAndDefault(opt.m_value);
      return *this;
    }

    // Changes the default value and the current one.
    void setValueAndDefault(const T& value) {
      setDefaultValue(value);
      setValue(value);
    }

    const char* section() const { return m_section->name(); }
    const char* id() const { return m_id; }
    const T& defaultValue() const { return m_default; }
    void setDefaultValue(const T& defValue) {
      m_default = defValue;
    }

    bool isDirty() const { return m_dirty; }
    void forceDirtyFlag() { m_dirty = true; }
    void cleanDirtyFlag() { m_dirty = false; }

    void setValue(const T& newValue) {
      if (m_value == newValue)
        return;

      BeforeChange(newValue);
      if (m_section)
        m_section->BeforeChange();

      m_value = newValue;
      m_dirty = true;

      AfterChange(newValue);
      if (m_section)
        m_section->AfterChange();
    }

    base::Signal1<void, const T&> BeforeChange;
    base::Signal1<void, const T&> AfterChange;

  private:
    Section* m_section;
    const char* m_id;
    T m_default;
    T m_value;
    bool m_dirty;

    Option();
    Option(const Option& opt);
  };

} // namespace app

#endif
