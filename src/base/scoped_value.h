// Aseprite Base Library
// Copyright (c) 2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef BASE_SCOPED_VALUE_H_INCLUDED
#define BASE_SCOPED_VALUE_H_INCLUDED
#pragma once

namespace base {

  template<typename T>
  class ScopedValue {
  public:
    ScopedValue(T& instance, const T& inScopeValue, const T& outScopeValue)
      : m_instance(instance)
      , m_outScopeValue(outScopeValue) {
      m_instance = inScopeValue;
    }

    ~ScopedValue() {
      m_instance = m_outScopeValue;
    }

  private:
    T& m_instance;
    T m_outScopeValue;
  };

} // namespace base

#endif
