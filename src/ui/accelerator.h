// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
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

  class Accelerator {
  public:
    Accelerator();
    Accelerator(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar);
    // Convert string like "Ctrl+Q" or "Alt+X" into an accelerator.
    explicit Accelerator(const std::string& str);

    bool isEmpty() const;
    std::string toString() const;

    bool check(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar) const;
    bool checkFromAllegroKeyArray() const;

    bool operator==(const Accelerator& other) const;
    bool operator!=(const Accelerator& other) const {
      return !operator==(other);
    }

    KeyModifiers modifiers() const { return m_modifiers; }
    KeyScancode scancode() const { return m_scancode; }
    int unicodeChar() const { return m_unicodeChar; }

  private:
    KeyModifiers m_modifiers;
    KeyScancode m_scancode;
    int m_unicodeChar;
  };

  typedef std::vector<Accelerator> Accelerators;

} // namespace ui

#endif
