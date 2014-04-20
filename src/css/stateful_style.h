// Aseprite CSS Library
// Copyright (C) 2013 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef CSS_STATEFUL_STYLE_H_INCLUDED
#define CSS_STATEFUL_STYLE_H_INCLUDED
#pragma once

#include "css/rule.h"
#include "css/state.h"
#include "css/style.h"
#include "css/value.h"

namespace css {

  class StatefulStyle {
  public:
    static const char kSeparator = ':';

    StatefulStyle() : m_style(NULL) { }
    StatefulStyle(const Style& style) : m_style(&style) { }
    StatefulStyle(const State& state) {
      operator+=(state);
    }
    StatefulStyle(const Style& style, const States& states) :
      m_style(&style) {
      operator+=(states);
    }

    const Style& style() const { return *m_style; }
    const States& states() const { return m_states; }

    StatefulStyle& setStyle(const Style& style) {
      m_style = &style;
      return *this;
    }
    StatefulStyle& operator+=(const State& state) {
      m_states += state;
      return *this;
    }
    StatefulStyle& operator+=(const States& states) {
      m_states += states;
      return *this;
    }

  private:
    const Style* m_style;
    States m_states;
  };

  inline StatefulStyle operator+(const Style& style, const State& state) {
    StatefulStyle styleState;
    styleState.setStyle(style);
    styleState += state;
    return styleState;
  }

  inline StatefulStyle operator+(const StatefulStyle& styleState, const State& state) {
    StatefulStyle styleState2 = styleState;
    styleState2 += state;
    return styleState2;
  }

  inline StatefulStyle operator+(const StatefulStyle& styleState, const States& states) {
    StatefulStyle styleState2 = styleState;
    styleState2 += states;
    return styleState2;
  }

} // namespace css

#endif
