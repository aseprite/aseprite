// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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
