// Aseprite Document Library
// Copyright (c) 2019  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_TILESETS_H_INCLUDED
#define DOC_TILESETS_H_INCLUDED
#pragma once

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

    tileset_index getIndex(Tileset *tileset) {
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

    void erase(const tileset_index tsi) {
      // Do not m_tilesets.erase() the tileset so other tilesets
      // indexes/IDs are kept intact.
      if (tsi == size()-1)
        m_tilesets.erase(--m_tilesets.end());
      else
        m_tilesets[tsi] = nullptr;
    }

  private:
    Array m_tilesets;
  };

} // namespace doc

#endif
