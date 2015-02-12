// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/with_palette.h"

#include "doc/palette.h"

namespace app {
namespace cmd {

using namespace doc;

WithPalette::WithPalette(Palette* palette)
  : m_paletteId(palette->id())
{
}

Palette* WithPalette::palette()
{
  return get<Palette>(m_paletteId);
}

} // namespace cmd
} // namespace app
