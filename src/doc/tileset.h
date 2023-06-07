// Aseprite Document Library
// Copyright (c) 2019-2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_TILESET_H_INCLUDED
#define DOC_TILESET_H_INCLUDED
#pragma once

#include "base/buffer.h"
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
    struct Tile {
      ImageRef image;
      UserData data;
      Tile() { }
      Tile(const ImageRef& image,
           const UserData& data) : image(image), data(data) { }
    };
    static UserData kNoUserData;
  public:
    typedef std::vector<Tile> Tiles;
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

    // Cached compressed tileset read/writen directly from .aseprite
    // files.
    void discardCompressedData();
    void setCompressedData(const base::buffer& buffer) const;
    const base::buffer& compressedData() const { return m_compressedData; }
    ObjectVersion compressedDataVersion() const { return m_compressedDataVersion; }

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
        return m_tiles[ti].image;
      else
        return ImageRef(nullptr);
    }
    void set(const tile_index ti,
             const ImageRef& image);

    UserData& getTileData(const tile_index ti) const {
      if (ti >= 0 && ti < size())
        return const_cast<UserData&>(m_tiles[ti].data);
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

    // Unused functions.
    bool operator==(const Tileset& other) const = delete;
    bool operator!=(const Tileset& other) const = delete;

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

    // Returns the number of tilemap layers that are referencing this tileset.
    int tilemapsCount() const;

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
    TilesetHashTable m_hash;
    std::string m_name;
    int m_baseIndex = 1;
    struct External {
      std::string filename;
      tileset_index tileset;
    } m_external;

    // This is a cached version of the compressed tileset data
    // directly read from an .aseprite file. It's used to save the
    // tileset as-is (without re-compressing). When we modify the
    // tileset (at least one tile), the compressed data is discarded,
    // and the recompressiong must be done.
    //
    // This was added to improve the performance of saving a sprite
    // when tilesets are not modified (generally useful when a sprite
    // contains several layers with tilesets).
    mutable base::buffer m_compressedData;
    mutable doc::ObjectVersion m_compressedDataVersion;
  };

} // namespace doc

#endif
