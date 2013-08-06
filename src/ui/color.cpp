// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

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
