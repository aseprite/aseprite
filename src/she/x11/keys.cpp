// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/keys.h"

namespace she {

bool x11_is_key_pressed(KeyScancode scancode)
{
  return false;                 // TODO
}

int x11_get_unicode_from_scancode(KeyScancode scancode)
{
  return 0;                     // TODO
}

} // namespace she
