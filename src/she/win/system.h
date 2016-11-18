// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_WIN_SYSTEM_H
#define SHE_WIN_SYSTEM_H
#pragma once

#include "she/common/system.h"
#include "she/win/pen.h"

namespace she {

bool win_is_key_pressed(KeyScancode scancode);
int win_get_unicode_from_scancode(KeyScancode scancode);

class WindowSystem : public CommonSystem {
public:
  WindowSystem() { }
  ~WindowSystem() { }

  PenAPI& penApi() {
    return m_penApi;
  }

  bool isKeyPressed(KeyScancode scancode) override {
    return win_is_key_pressed(scancode);
  }

  int getUnicodeFromScancode(KeyScancode scancode) override {
    return win_get_unicode_from_scancode(scancode);
  }

private:
  PenAPI m_penApi;
};

} // namespace she

#endif
