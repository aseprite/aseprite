// Aseprite UI Library
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SHORTCUT_H_INCLUDED
#define UI_SHORTCUT_H_INCLUDED
#pragma once

#include "ui/keys.h"
#include "ui/mouse_button.h"

#include <algorithm>
#include <string>
#include <vector>

namespace ui {

extern const char* kWinKeyName;

class Shortcut {
public:
  Shortcut();
  Shortcut(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar);
  Shortcut(KeyModifiers modifiers, MouseButton mouseButton);
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
  MouseButton mouseButton() const { return m_mouseButton; }

  void removeModifiers() { m_modifiers = kKeyNoneModifier; }

private:
  KeyModifiers m_modifiers = kKeyNoneModifier;
  KeyScancode m_scancode = kKeyNil;
  int m_unicodeChar = 0;
  MouseButton m_mouseButton = kButtonNone;
};

template<typename T>
class ShortcutsT {
public:
  using List = std::vector<T>;
  using iterator = typename List::iterator;
  using const_iterator = typename List::const_iterator;

  iterator begin() { return m_list.begin(); }
  iterator end() { return m_list.end(); }
  const_iterator begin() const { return m_list.begin(); }
  const_iterator end() const { return m_list.end(); }

  bool empty() const { return m_list.empty(); }
  std::size_t size() const { return m_list.size(); }

  const T& front() const { return m_list.front(); }

  const T& operator[](int index) const { return m_list[index]; }

  T& operator[](int index) { return m_list[index]; }

  void clear() { m_list.clear(); }

  bool has(const T& shortcut) const { return (std::find(begin(), end(), shortcut) != end()); }

  void push_back(const T& shortcut) { m_list.push_back(shortcut); }

  void add(const T& shortcut)
  {
    if (!has(shortcut))
      m_list.push_back(shortcut);
  }

  void remove(const T& shortcut)
  {
    auto it = std::find(begin(), end(), shortcut);
    if (it != end())
      m_list.erase(it);
  }

  iterator erase(const iterator& it) { return m_list.erase(it); }

private:
  List m_list;
};

using Shortcuts = ShortcutsT<Shortcut>;

} // namespace ui

#endif
