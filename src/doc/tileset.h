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

  class Remap;
  class Sprite;

  class Tileset : public Object {
  public:
    typedef std::vector<ImageRef> Tiles;
    typedef Tiles::iterator iterator;
    typedef Tiles::const_iterator const_iterator;

    Tileset(Sprite* sprite,
            const Grid& grid,
            const tileset_index ntiles);

    static Tileset* MakeCopyWithSameImages(const Tileset* tileset);
    static Tileset* MakeCopyCopyingImages(const Tileset* tileset);

    Sprite* sprite() const { return m_sprite; }
    const Grid& grid() const { return m_grid; }
    void setOrigin(const gfx::Point& pt);

    const std::string& name() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    int getMemSize() const override;

    iterator begin() { return m_tiles.begin(); }
    iterator end() { return m_tiles.end(); }
    const_iterator begin() const { return m_tiles.begin(); }
    const_iterator end() const { return m_tiles.end(); }
    tile_index size() const { return tile_index(m_tiles.size()); }
    void resize(const tile_index ntiles);
    void remap(const Remap& remap);

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

    // Linked with an external file
    void setExternal(const std::string& filename,
                     const tileset_index& tsi);
    const std::string& externalFilename() const { return m_external.filename; }
    tileset_index externalTileset() const { return m_external.tileset; }

    bool operator==(const Tileset& other) const {
      // TODO compare the all grid members
      return (m_grid.tileSize() == other.m_grid.tileSize() &&
              m_tiles == other.m_tiles &&
              m_name == other.m_name);
    }

    bool operator!=(const Tileset& other) const {
      return !operator==(other);
    }

    // Returns a new empty tile with the tileset specs.
    ImageRef makeEmptyTile();

  private:
    Sprite* m_sprite;
    Grid m_grid;
    Tiles m_tiles;
    std::string m_name;
    struct External {
      std::string filename;
      tileset_index tileset;
    } m_external;
  };

} // namespace doc

#endif
