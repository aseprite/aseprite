// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_ACCELERATOR_H_INCLUDED
#define UI_ACCELERATOR_H_INCLUDED

#include <string>
#include <vector>

namespace ui {

  class Accelerator
  {
  public:
    void addKey(int shifts, int ascii, int scancode);

    // Adds keys from strings like "<Ctrl+Q> <ESC>"
    void addKeysFromString(const char* string);

    bool isEmpty() const { return m_combos.empty(); }
    std::string toString();

    bool check(int shifts, int ascii, int scancode);
    bool checkFromAllegroKeyArray();

  private:
    struct KeyCombo {
      int shifts;
      int ascii;
      int scancode;
      std::string toString();
    };

    typedef std::vector<KeyCombo> KeyCombos;
    KeyCombos m_combos;
  };

} // namespace ui

#endif
