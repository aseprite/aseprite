// LAF OS Library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef OS_OSX_SYSTEM_H
#define OS_OSX_SYSTEM_H
#pragma once

#include "os/common/system.h"

namespace os {

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

} // namespace os

#endif
