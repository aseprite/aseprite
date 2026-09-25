// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_INLINE_SEARCH_H
#define APP_UI_INLINE_SEARCH_H

#include "base/time.h"
#include "ui/message.h"

namespace app {

using namespace ui;

// Utility class to implement inline search on widgets that have several labeled items
class InlineSearch {
public:
  InlineSearch() = default;

  bool search(const KeyMessage* keyMsg);
  const std::string& string() const;
  bool match(const std::string& string) const;
  bool wasInTime() const;

private:
  base::tick_t m_lastCharTick;
  std::string m_string;
  bool m_inTime = false;
};

} // namespace app

#endif // APP_UI_INLINE_SEARCH_H
