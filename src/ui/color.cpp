// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/color.h"

#include <allegro.h>

namespace ui {

int to_system(Color color)
{
  if (is_transparent(color))
    return -1;
  else
    return makecol(getr(color),
                   getg(color),
                   getb(color));
}

} // namespace ui
