// Aseprite Document Library
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SPRITES_H_INCLUDED
#define DOC_SPRITES_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/color_mode.h"
#include "doc/object_id.h"
#include "doc/sprite.h"

#include <vector>

namespace doc {
  class Document;

  class Sprites {
  public:
    typedef std::vector<Sprite*>::iterator iterator;
    typedef std::vector<Sprite*>::const_iterator const_iterator;

    Sprites(Document* doc);
    ~Sprites();

    iterator begin() { return m_sprites.begin(); }
    iterator end() { return m_sprites.end(); }
    const_iterator begin() const { return m_sprites.begin(); }
    const_iterator end() const { return m_sprites.end(); }

    Sprite* front() const { return m_sprites.front(); }
    Sprite* back() const { return m_sprites.back(); }

    int size() const { return (int)m_sprites.size(); }
    bool empty() const { return m_sprites.empty(); }

    Sprite* add(Sprite* spr);
    void remove(Sprite* spr);
    void move(Sprite* spr, int index);

    Sprite* operator[](int index) const { return m_sprites[index]; }

  private:
    // Deletes all sprites in the list (calling "delete" operation).
    void deleteAll();

    Document* m_doc;
    std::vector<Sprite*> m_sprites;

    DISABLE_COPYING(Sprites);
  };

} // namespace doc

#endif
