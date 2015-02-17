// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_FRAME_TAGS_H_INCLUDED
#define DOC_FRAME_TAGS_H_INCLUDED
#pragma once

#include "base/disable_copying.h"

#include <vector>

namespace doc {

  class FrameTag;
  class Sprite;

  class FrameTags {
    typedef std::vector<FrameTags> List;

  public:
    typedef List::iterator iterator;

    FrameTags(Sprite* sprite);

    Sprite* sprite() { return m_sprite; }

    void add(FrameTag* tag);
    void remove(FrameTag* tag);

    iterator begin() { return m_tags.begin(); }
    iterator end() { return m_tags.end(); }

  private:
    Sprite* m_sprite;
    List m_tags;

    DISABLE_COPYING(FrameTags);
  };

} // namespace doc

#endif
