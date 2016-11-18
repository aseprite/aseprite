// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_OSX_SYSTEM_H
#define SHE_OSX_SYSTEM_H
#pragma once

#include "she/common/system.h"

namespace she {

bool osx_is_key_pressed(KeyScancode scancode);
int osx_get_unicode_from_scancode(KeyScancode scancode);

class OSXSystem : public CommonSystem {
public:
  bool isKeyPressed(KeyScancode scancode) override {
    return osx_is_key_pressed(scancode);
  }

  int getUnicodeFromScancode(KeyScancode scancode) override {
    return osx_get_unicode_from_scancode(scancode);
  }
};

} // namespace she

#endif
