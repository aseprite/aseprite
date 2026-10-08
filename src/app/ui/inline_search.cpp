// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "base/string.h"

#include "app/ui/inline_search.h"

namespace app {
bool InlineSearch::search(const KeyMessage* keyMsg)
{
  if (!keyMsg->isDeadKey() && keyMsg->unicodeChar() >= 32) {
    m_inTime = (base::current_tick() - m_lastCharTick) < 1500;
    if (!m_inTime) {
      m_string.clear();
      m_lastCharTick = base::current_tick();
    }

    m_string += base::string_to_lower(base::codepoint_to_utf8(keyMsg->unicodeChar()));
    return true;
  }

  return false;
}

const std::string& InlineSearch::string() const
{
  return m_string;
}

bool InlineSearch::match(const std::string& string) const
{
  return base::string_to_lower(string).find(m_string) == 0;
}

bool InlineSearch::wasInTime() const
{
  return m_inTime;
}
} // namespace app
