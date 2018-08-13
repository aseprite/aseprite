// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_X11_SYSTEM_H
#define SHE_X11_SYSTEM_H
#pragma once

#include "she/common/system.h"
#include "she/x11/keys.h"

namespace she {

class X11System : public CommonSystem {
public:
  bool isKeyPressed(KeyScancode scancode) override {
    return x11_is_key_pressed(scancode);
  }

  int getUnicodeFromScancode(KeyScancode scancode) override {
    return x11_get_unicode_from_scancode(scancode);
  }
};

} // namespace she

#endif
