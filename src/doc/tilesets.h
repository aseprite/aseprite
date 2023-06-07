// Aseprite Document Library
// Copyright (c) 2019-2023  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_TILESETS_H_INCLUDED
#define DOC_TILESETS_H_INCLUDED
#pragma once

#include "doc/layer_tilemap.h"
#include "doc/sprite.h"
#include "doc/tileset.h"

#include <vector>

namespace doc {

  class Tilesets : public Object {
  public:
    typedef std::vector<Tileset*> Array;
    typedef Array::iterator iterator;
    typedef Array::const_iterator const_iterator;

    Tilesets();
    ~Tilesets();

    int getMemSize() const override;

    iterator begin() { return m_tilesets.begin(); }
    iterator end() { return m_tilesets.end(); }
    const_iterator begin() const { return m_tilesets.begin(); }
    const_iterator end() const { return m_tilesets.end(); }
    tileset_index size() const { return (tileset_index)m_tilesets.size(); }

    tileset_index add(Tileset* tileset);

    Tileset* get(const tileset_index tsi) {
      if (tsi < size())
        return m_tilesets[tsi];
      else
        return nullptr;
    }

    tileset_index getIndex(const Tileset *tileset) {
      for (tileset_index i = 0; i < size(); ++i) {
        if (m_tilesets[i] == tileset) {
          return i;
        }
      }
      return -1;
    }

    void set(const tileset_index tsi, Tileset* tileset) {
      if (tsi >= m_tilesets.size())
        m_tilesets.resize(tsi+1, nullptr);
      m_tilesets[tsi] = tileset;
    }

    void add(const tileset_index tsi, Tileset* tileset) {
      if (tsi >= m_tilesets.size()) {
        m_tilesets.push_back(tileset);
      }
      else {
        m_tilesets.insert(m_tilesets.begin()+tsi, tileset);
        // Update tileset indexes of the affected tilemaps. We have to shift the indexes
        // for all the tilemaps pointing to a tileset index equals or greater than the added one.
        shiftTilesetIndexes(tileset->sprite(), tsi, 1);
      }
    }

    void erase(const tileset_index tsi) {
      // When tsi is the last one, other tilemaps tilesets
      // indexes are not affected.
      if (tsi == size()-1) {
        m_tilesets.erase(--m_tilesets.end());
      }
      else {
        auto ts = m_tilesets[tsi];
        m_tilesets.erase(m_tilesets.begin()+tsi);
        // Update tileset indexes of the affected tilemaps. We have to shift the indexes
        // for all the tilemaps pointing to a tileset index greater than the deleted one.
        shiftTilesetIndexes(ts->sprite(), tsi+1, -1);
      }
    }

  private:
    Array m_tilesets;

    void shiftTilesetIndexes(Sprite *sprite, tileset_index pos, int n) {
      for (auto layer : sprite->allTilemaps()) {
        auto tilemap = static_cast<LayerTilemap*>(layer);
        if (tilemap->tilesetIndex() >= pos) {
          tilemap->setTilesetIndex(tilemap->tilesetIndex()+n);
        }
      }
    }
  };

} // namespace doc

#endif
