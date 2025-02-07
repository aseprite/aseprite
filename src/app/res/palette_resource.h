// Aseprite
// Copyright (C) 2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_RES_PALETTE_RESOURCE_H_INCLUDED
#define APP_RES_PALETTE_RESOURCE_H_INCLUDED
#pragma once

#include "app/res/resource.h"
#include "doc/palette.h"

#include <memory>

namespace doc {
class Palette;
}

namespace app {

class PaletteResource : public Resource {
public:
  PaletteResource(const std::string& id,
                  const std::string& path,
                  std::unique_ptr<doc::Palette>&& palette)
    : m_id(id)
    , m_path(path)
    , m_palette(std::move(palette))
  {
  }
  virtual ~PaletteResource() {}
  virtual const std::string& id() const override { return m_id; }
  virtual const std::string& path() const override { return m_path; }
  virtual const doc::Palette* palette() { return m_palette.get(); }

private:
  std::string m_id;
  std::string m_path;
  std::unique_ptr<doc::Palette> m_palette;
};

} // namespace app

#endif
