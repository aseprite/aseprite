// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

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

    // Adds keys from strings like "<Ctrl+Q> <ESC>"
    void addKeysFromString(const char* string);

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
