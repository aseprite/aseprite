// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/keys.h"

#include <allegro.h>

int ui::scancode_to_unicode(KeyScancode scancode)
{
  return ::scancode_to_ascii(scancode);
}
