// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/keys.h"

#include <allegro.h>

int ui::scancode_to_ascii(KeyScancode scancode)
{
  return ::scancode_to_ascii(scancode);
}
