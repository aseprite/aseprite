// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_MATCH_WORDS_H_INCLUDED
#define APP_MATCH_WORDS_H_INCLUDED
#pragma once

#include "base/split_string.h"
#include "base/string.h"

#include <string>
#include <vector>

namespace app {

class MatchWords {
public:
  MatchWords(const std::string& search = {})
  {
    base::split_string(base::string_to_lower(search), m_parts, " ");
  }

  bool operator()(const std::string& item) const
  {
    std::string lowerItem = base::string_to_lower(item);
    std::size_t matches = 0;

    for (const auto& part : m_parts) {
      if (lowerItem.find(part) != std::string::npos)
        ++matches;
    }

    return (matches == m_parts.size());
  }

private:
  std::vector<std::string> m_parts;
};

} // namespace app

#endif
