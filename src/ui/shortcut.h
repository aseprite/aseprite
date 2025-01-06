// Aseprite UI Library
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SHORTCUT_H_INCLUDED
#define UI_SHORTCUT_H_INCLUDED
#pragma once

#include <string>
#include <vector>

#include "ui/keys.h"

namespace ui {

extern const char* kWinKeyName;

class Shortcut {
public:
  Shortcut();
  Shortcut(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar);
  // Convert string like "Ctrl+Q" or "Alt+X" into an shortcut.
  explicit Shortcut(const std::string& str);

  bool isEmpty() const;
  std::string toString() const;

  bool isPressed(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar) const;

  // Returns true if the key is pressed and ONLY its modifiers are
  // pressed.
  bool isPressed() const;

  // Returns true if the key + its modifiers are pressed (other
  // modifiers are allowed too).
  bool isLooselyPressed() const;

  bool operator==(const Shortcut& other) const;
  bool operator!=(const Shortcut& other) const { return !operator==(other); }

  KeyModifiers modifiers() const { return m_modifiers; }
  KeyScancode scancode() const { return m_scancode; }
  int unicodeChar() const { return m_unicodeChar; }

private:
  KeyModifiers m_modifiers;
  KeyScancode m_scancode;
  int m_unicodeChar;
};

class Shortcuts {
public:
  typedef std::vector<Shortcut> List;
  typedef List::iterator iterator;
  typedef List::const_iterator const_iterator;

  iterator begin() { return m_list.begin(); }
  iterator end() { return m_list.end(); }
  const_iterator begin() const { return m_list.begin(); }
  const_iterator end() const { return m_list.end(); }

  bool empty() const { return m_list.empty(); }
  std::size_t size() const { return m_list.size(); }

  const ui::Shortcut& front() const { return m_list.front(); }

  const ui::Shortcut& operator[](int index) const { return m_list[index]; }

  ui::Shortcut& operator[](int index) { return m_list[index]; }

  void clear() { m_list.clear(); }
  bool has(const Shortcut& shortcut) const;
  void add(const Shortcut& shortcut);
  void remove(const Shortcut& shortcut);

private:
  List m_list;
};

} // namespace ui

#endif
