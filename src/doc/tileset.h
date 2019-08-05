// Aseprite Document Library
// Copyright (c) 2019  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_TILESET_H_INCLUDED
#define DOC_TILESET_H_INCLUDED
#pragma once

#include "doc/grid.h"
#include "doc/image_ref.h"
#include "doc/object.h"
#include "doc/tile.h"

#include <string>
#include <vector>

namespace doc {

  class Sprite;

  class Tileset : public Object {
  public:
    typedef std::vector<ImageRef> Tiles;
    typedef Tiles::iterator iterator;

    Tileset(Sprite* sprite,
            const Grid& grid,
            const tileset_index ntiles);

    Sprite* sprite() const { return m_sprite; }
    const Grid& grid() const { return m_grid; }
    void setOrigin(const gfx::Point& pt);

    const std::string& name() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    int getMemSize() const override;

    iterator begin() { return m_tiles.begin(); }
    iterator end() { return m_tiles.end(); }
    tile_index size() const { return tile_index(m_tiles.size()); }
    void resize(const tile_index ntiles);

    ImageRef get(const tile_index ti) const {
      if (ti < size())
        return m_tiles[ti];
      else
        return ImageRef(nullptr);
    }

    void set(const tile_index ti,
             const ImageRef& image) {
      m_tiles[ti] = image;
    }

    tile_index add(const ImageRef& image) {
      m_tiles.push_back(image);
      return tile_t(m_tiles.size()-1);
    }

    void insert(const tile_index ti,
                const ImageRef& image) {
      ASSERT(ti <= size());
      m_tiles.insert(m_tiles.begin()+ti, image);
    }

    void erase(const tile_index ti) {
      ASSERT(ti >= 0 && ti < size());
      m_tiles.erase(m_tiles.begin()+ti);
    }

  private:
    Sprite* m_sprite;
    Grid m_grid;
    Tiles m_tiles;
    std::string m_name;
  };

} // namespace doc

#endif
