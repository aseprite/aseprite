// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_FRAME_TAGS_H_INCLUDED
#define DOC_FRAME_TAGS_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/frame.h"
#include "doc/object_id.h"

#include <string>
#include <vector>

namespace doc {

  class FrameTag;
  class Sprite;

  class FrameTags {
    typedef std::vector<FrameTag*> List;

  public:
    typedef List::iterator iterator;
    typedef List::const_iterator const_iterator;

    FrameTags(Sprite* sprite);
    ~FrameTags();

    Sprite* sprite() { return m_sprite; }

    void add(FrameTag* tag);
    void remove(FrameTag* tag);

    FrameTag* getByName(const std::string& name) const;
    FrameTag* getById(const ObjectId id) const;

    iterator begin() { return m_tags.begin(); }
    iterator end() { return m_tags.end(); }
    const_iterator begin() const { return m_tags.begin(); }
    const_iterator end() const { return m_tags.end(); }

    std::size_t size() const { return m_tags.size(); }
    bool empty() const { return m_tags.empty(); }

    FrameTag* innerTag(frame_t frame) const;
    FrameTag* outerTag(frame_t frame) const;

  private:
    Sprite* m_sprite;
    List m_tags;

    DISABLE_COPYING(FrameTags);
  };

} // namespace doc

#endif
