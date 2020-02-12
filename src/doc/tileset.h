// Aseprite Document Library
// Copyright (c) 2019-2020  Igara Studio S.A.
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
#include "doc/tileset_hash_table.h"

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
             const ImageRef& image);

    tile_index add(const ImageRef& image);
    void insert(const tile_index ti,
                const ImageRef& image);
    void erase(const tile_index ti);

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

    // If there is a tile in the set that matches the pixels of the
    // given "tileImage", this function returns the index of that
    // tile. Returns tile_i_notile if the image is not in the tileset.
    tile_index findTileIndex(const ImageRef& tileImage);

  private:
    void removeFromHash(const tile_index ti);

    Sprite* m_sprite;
    Grid m_grid;
    Tiles m_tiles;
    TilesetHashTable m_hash;
    std::string m_name;
    struct External {
      std::string filename;
      tileset_index tileset;
    } m_external;
  };

} // namespace doc

#endif
