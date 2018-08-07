// SHE library
// Copyright (C) 2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_X11_KEYS_INCLUDED
#define SHE_X11_KEYS_INCLUDED
#pragma once

#include "she/keys.h"
#include <X11/X.h>

namespace she {

  KeyScancode x11_keysym_to_scancode(KeySym keysym);
  bool x11_is_key_pressed(KeyScancode scancode);
  int x11_get_unicode_from_scancode(KeyScancode scancode);

} // namespace she

#endif
