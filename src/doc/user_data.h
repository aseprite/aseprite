// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_USER_DATA_H_INCLUDED
#define DOC_USER_DATA_H_INCLUDED
#pragma once

#include "doc/color.h"

#include <string>

namespace doc {

  class UserData {
  public:
    UserData() : m_color(0) {
    }

    size_t size() const { return m_text.size(); }
    bool isEmpty() const {
      return m_text.empty() && !doc::rgba_geta(m_color);
    }

    const std::string& text() const { return m_text; }
    color_t color() const { return m_color; }

    void setText(const std::string& text) { m_text = text; }
    void setColor(color_t color) { m_color = color; }

    bool operator==(const UserData& other) const {
      return (m_text == other.m_text &&
              m_color == other.m_color);
    }

    bool operator!=(const UserData& other) const {
      return !operator==(other);
    }

  private:
    std::string m_text;
    color_t m_color;
  };

} // namespace doc

#endif
