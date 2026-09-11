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

#include <algorithm>
#include <string>
#include <vector>

namespace app {

class MatchWords {
public:
  MatchWords(const std::string& search = {})
  {
    base::split_string(base::string_to_lower(search), m_parts, " ");
    if (m_parts.size() > 1 && m_parts.back().empty())
      m_parts.pop_back(); // Avoid an empty part when the search text is something like "hello "
  }

  // Finds a match with the given word using Levenshtein distance
  // Expects lower case input, unlike operator()
  bool fuzzy(const std::string& word, size_t threshold = 3) const
  {
    std::size_t matches = 0;
    for (const auto& part : m_parts) {
      if (levenshteinDistance(part, word) < threshold)
        ++matches;
    }
    return (matches == m_parts.size());
  }

  // Finds a match with any of the given words using Levenshtein distance
  // Expects lower case input, unlike operator()
  bool fuzzyWords(const std::vector<std::string>& lowerWords, size_t threshold = 3) const
  {
    std::size_t matches = 0;
    for (const auto& part : m_parts) {
      for (const auto& word : lowerWords) {
        if (levenshteinDistance(part, word) < threshold)
          ++matches;
      }
    }

    return (matches == m_parts.size());
  }

  bool operator()(const std::string& item) const
  {
    const std::string& lowerItem = base::string_to_lower(item);
    return word(lowerItem);
  }

  bool word(const std::string& lowerItem) const
  {
    std::size_t matches = 0;

    for (const auto& part : m_parts) {
      if (lowerItem.find(part) != std::string::npos)
        ++matches;
    }

    return (matches == m_parts.size());
  }

  static size_t levenshteinDistance(const std::string_view& word1, const std::string_view& word2)
  {
    if (word1 == word2)
      return 0;

    std::vector distance1(word2.size() + 1, 0);
    std::vector distance2(word2.size() + 1, 0);

    for (int i = 0; i <= word2.size(); ++i)
      distance1[i] = i;

    for (int i = 0; i < word1.size(); ++i) {
      distance2[0] = i + 1;

      for (int j = 0; j < word2.size(); ++j) {
        const int deletionCost = distance1[j + 1] + 1;
        const int insertionCost = distance2[j] + 1;
        const int substitutionCost = (word1[i] == word2[j]) ? distance1[j] : distance1[j] + 1;
        distance2[j + 1] = std::min({ deletionCost, insertionCost, substitutionCost });
      }

      std::swap(distance1, distance2);
    }

    return distance1[word2.size()];
  }

private:
  std::vector<std::string> m_parts;
};

} // namespace app

#endif
