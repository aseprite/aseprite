/* Aseprite
 * Copyright (C) 2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_RES_PALETTE_RESOURCE_H_INCLUDED
#define APP_RES_PALETTE_RESOURCE_H_INCLUDED
#pragma once

#include "app/res/resource.h"

namespace doc {
  class Palette;
}

namespace app {

  class PaletteResource : public Resource {
  public:
    PaletteResource(doc::Palette* palette, const std::string& name)
      : m_palette(palette)
      , m_name(name) {
    }
    virtual ~PaletteResource() { }
    virtual doc::Palette* palette() { return m_palette; }
    virtual const std::string& name() const override { return m_name; }

  private:
    doc::Palette* m_palette;
    std::string m_name;
  };

} // namespace app

#endif
