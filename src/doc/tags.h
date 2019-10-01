// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_TAGS_H_INCLUDED
#define DOC_TAGS_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/frame.h"
#include "doc/object_id.h"

#include <string>
#include <vector>

namespace doc {

  class Tag;
  class Sprite;

  class Tags {
    typedef std::vector<Tag*> List;

  public:
    typedef List::iterator iterator;
    typedef List::const_iterator const_iterator;

    Tags(Sprite* sprite);
    ~Tags();

    Sprite* sprite() { return m_sprite; }

    void add(Tag* tag);
    void remove(Tag* tag);

    Tag* getByName(const std::string& name) const;
    Tag* getById(const ObjectId id) const;

    iterator begin() { return m_tags.begin(); }
    iterator end() { return m_tags.end(); }
    const_iterator begin() const { return m_tags.begin(); }
    const_iterator end() const { return m_tags.end(); }

    std::size_t size() const { return m_tags.size(); }
    bool empty() const { return m_tags.empty(); }

    Tag* innerTag(const frame_t frame) const;
    Tag* outerTag(const frame_t frame) const;

  private:
    Sprite* m_sprite;
    List m_tags;

    DISABLE_COPYING(Tags);
  };

} // namespace doc

#endif
