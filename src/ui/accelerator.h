// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_ACCELERATOR_H_INCLUDED
#define UI_ACCELERATOR_H_INCLUDED
#pragma once

#include <string>
#include <vector>

#include "ui/keys.h"

namespace ui {

extern const char* kWinKeyName;

// TODO rename this class to Shortcut
class Accelerator {
public:
  Accelerator();
  Accelerator(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar);
  // Convert string like "Ctrl+Q" or "Alt+X" into an accelerator.
  explicit Accelerator(const std::string& str);

  bool isEmpty() const;
  std::string toString() const;

  bool isPressed(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar) const;

  // Returns true if the key is pressed and ONLY its modifiers are
  // pressed.
  bool isPressed() const;

  // Returns true if the key + its modifiers are pressed (other
  // modifiers are allowed too).
  bool isLooselyPressed() const;

  bool operator==(const Accelerator& other) const;
  bool operator!=(const Accelerator& other) const { return !operator==(other); }

  KeyModifiers modifiers() const { return m_modifiers; }
  KeyScancode scancode() const { return m_scancode; }
  int unicodeChar() const { return m_unicodeChar; }

private:
  KeyModifiers m_modifiers;
  KeyScancode m_scancode;
  int m_unicodeChar;
};

// TODO rename this class to Shortcuts
class Accelerators {
public:
  typedef std::vector<Accelerator> List;
  typedef List::iterator iterator;
  typedef List::const_iterator const_iterator;

  iterator begin() { return m_list.begin(); }
  iterator end() { return m_list.end(); }
  const_iterator begin() const { return m_list.begin(); }
  const_iterator end() const { return m_list.end(); }

  bool empty() const { return m_list.empty(); }
  std::size_t size() const { return m_list.size(); }

  const ui::Accelerator& front() const { return m_list.front(); }

  const ui::Accelerator& operator[](int index) const { return m_list[index]; }

  ui::Accelerator& operator[](int index) { return m_list[index]; }

  void clear() { m_list.clear(); }
  bool has(const Accelerator& accel) const;
  void add(const Accelerator& accel);
  void remove(const Accelerator& accel);

private:
  List m_list;
};

} // namespace ui

#endif
