// Aseprite UI Library
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SHORTCUT_H_INCLUDED
#define UI_SHORTCUT_H_INCLUDED
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "ui/keys.h"

namespace ui {

extern const char* kWinKeyName;

class Shortcut {
public:
  Shortcut();
  Shortcut(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar);
  // Convert string like "Ctrl+Q" or "Alt+X" or multi-key "Ctrl+K,Ctrl+O" into a shortcut.
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

  // Multi-key sequence support
  bool isSequence() const { return !m_sequence.empty(); }
  const std::vector<Shortcut>& sequence() const { return m_sequence; }
  std::size_t sequenceSize() const { return m_sequence.empty() ? 1 : m_sequence.size(); }
  
  // Get a specific key in the sequence (0 for single key or first in sequence)
  const Shortcut& getKeyAt(std::size_t index) const;
  
  // Create a shortcut sequence from multiple shortcuts
  static Shortcut MakeSequence(const std::vector<Shortcut>& keys);

private:
  KeyModifiers m_modifiers;
  KeyScancode m_scancode;
  int m_unicodeChar;
  
  // For multi-key sequences like "Ctrl+K Ctrl+O"
  // If empty, this is a single-key shortcut (using m_modifiers, m_scancode, m_unicodeChar)
  // If non-empty, this is a sequence and the individual fields are ignored
  std::vector<Shortcut> m_sequence;
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
