// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
