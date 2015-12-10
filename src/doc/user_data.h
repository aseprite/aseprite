// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_USER_DATA_H_INCLUDED
#define DOC_USER_DATA_H_INCLUDED
#pragma once

#include <string>

namespace doc {

  class UserData {
  public:
    size_t size() const { return m_text.size(); }
    bool isEmpty() const { return m_text.empty(); }

    const std::string& text() const { return m_text; }
    void setText(const std::string& text) { m_text = text; }

    bool operator==(const UserData& other) const {
      return (m_text == other.m_text);
    }

    bool operator!=(const UserData& other) const {
      return !operator==(other);
    }

  private:
    std::string m_text;
  };

} // namespace doc

#endif
