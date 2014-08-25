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

  class Accelerator
  {
  public:
    void addKey(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar);

    // Adds keys from strings like "Ctrl+Q" or "Alt+X"
    void addKeysFromString(const std::string& str);

    bool isEmpty() const { return m_combos.empty(); }
    std::string toString();

    bool check(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar);
    bool checkFromAllegroKeyArray();

  private:
    struct KeyCombo {
      KeyModifiers modifiers;
      KeyScancode scancode;
      int unicodeChar;

      std::string toString();
    };

    typedef std::vector<KeyCombo> KeyCombos;
    KeyCombos m_combos;
  };

} // namespace ui

#endif
