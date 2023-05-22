// Aseprite Document Library
// Copyright (C) 2022  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_WITH_FLAGS_H_INCLUDED
#define DOC_WITH_FLAGS_H_INCLUDED
#pragma once

namespace doc {

  template<typename T>
  class WithFlags {
  public:
    WithFlags(T flags = {}) : m_flags(flags) { }

    T flags() const {
      return m_flags;
    }

    bool hasFlags(T flags) const {
      return (int(m_flags) & int(flags)) == int(flags);
    }

    void setFlags(T flags) {
      m_flags = flags;
    }

    void switchFlags(T flags, bool state) {
      if (state)
        m_flags = T(int(m_flags) | int(flags));
      else
        m_flags = T(int(m_flags) & ~int(flags));
    }

  private:
    T m_flags;
  };

} // namespace doc

#endif
