/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#ifndef APP_PREF_OPTION_H_INCLUDED
#define APP_PREF_OPTION_H_INCLUDED
#pragma once

#include "base/signal.h"
#include "base/disable_copying.h"

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
    DISABLE_COPYING(Option);
  };

} // namespace app

#endif
