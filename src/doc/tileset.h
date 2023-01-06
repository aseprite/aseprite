// Aseprite Document Library
// Copyright (c) 2019-2023  Igara Studio S.A.
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
#include "doc/with_user_data.h"

#include <string>
#include <vector>

namespace doc {

  class Remap;
  class Sprite;

  class Tileset : public WithUserData {
    static UserData kNoUserData;
  public:
    typedef std::vector<ImageRef> Tiles;
    typedef std::vector<UserData> Datas;
    typedef Tiles::iterator iterator;
    typedef Tiles::const_iterator const_iterator;

    // Creates a new tileset with "ntiles". The first tile will be
    // always the empty tile. So ntiles must be > 1 to contain at
    // least one non-empty tile.
    Tileset(Sprite* sprite,
            const Grid& grid,
            const tileset_index ntiles);

    static Tileset* MakeCopyWithoutImages(const Tileset* tileset);
    static Tileset* MakeCopyWithSameImages(const Tileset* tileset);
    static Tileset* MakeCopyCopyingImages(const Tileset* tileset);

    Sprite* sprite() const { return m_sprite; }
    const Grid& grid() const { return m_grid; }

    const std::string& name() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    int baseIndex() const { return m_baseIndex; }
    void setBaseIndex(int index) { m_baseIndex = index; }

    int getMemSize() const override;

    iterator begin() { return m_tiles.begin(); }
    iterator end() { return m_tiles.end(); }
    const_iterator begin() const { return m_tiles.begin(); }
    const_iterator end() const { return m_tiles.end(); }
    tile_index size() const { return tile_index(m_tiles.size()); }
    void resize(const tile_index ntiles);
    void remap(const Remap& remap);

    ImageRef get(const tile_index ti) const {
      if (ti >= 0 && ti < size())
        return m_tiles[ti];
      else
        return ImageRef(nullptr);
    }
    void set(const tile_index ti,
             const ImageRef& image);

    UserData& getTileData(const tile_index ti) const {
      if (ti >= 0 && ti < size())
        return const_cast<UserData&>(m_datas[ti]);
      else
        return kNoUserData;
    }
    void setTileData(const tile_index ti,
                     const UserData& userData);

    tile_index add(const ImageRef& image,
                   const UserData& userData = UserData());
    void insert(const tile_index ti,
                const ImageRef& image,
                const UserData& userData = UserData());
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
    // given "tileImage", this function returns true and the index of
    // that tile in the "ti" parameter. Returns false if the image is
    // not in the tileset.
    //
    // Warning: Use preprocess_transparent_pixels() with tileImage
    // before calling this function.
    bool findTileIndex(const ImageRef& tileImage,
                       tile_index& ti);

    // Must be called when a tile image was modified externally, so
    // the hash elements are re-calculated for that specific tile.
    void notifyTileContentChange(const tile_index ti);

    // Called when the mask color of the sprite is modified, so we
    // have to regenerate the empty tile with that new mask color.
    void notifyRegenerateEmptyTile();

#ifdef _DEBUG
    void assertValidHashTable();
#endif

  private:
    void removeFromHash(const tile_index ti,
                        const bool adjustIndexes);
    void hashImage(const tile_index ti,
                   const ImageRef& tileImage);
    void rehash();
    TilesetHashTable& hashTable();

    Sprite* m_sprite;
    Grid m_grid;
    Tiles m_tiles;
    Datas m_datas;
    TilesetHashTable m_hash;
    std::string m_name;
    int m_baseIndex = 1;
    struct External {
      std::string filename;
      tileset_index tileset;
    } m_external;
  };

} // namespace doc

#endif
