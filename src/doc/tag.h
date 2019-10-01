// Aseprite Document Library
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_TAG_H_INCLUDED
#define DOC_TAG_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/anidir.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/object.h"

#include <string>

namespace doc {
  class Tags;

  class Tag : public Object {
  public:
    Tag(frame_t from, frame_t to);
    Tag(const Tag& other);
    ~Tag();

    Tags* owner() const { return m_owner; }
    frame_t fromFrame() const { return m_from; }
    frame_t toFrame() const { return m_to; }
    frame_t frames() const { return m_to - m_from + 1; }
    const std::string& name() const { return m_name; }
    color_t color() const { return m_color; }
    AniDir aniDir() const { return m_aniDir; }

    void setFrameRange(frame_t from, frame_t to);
    void setName(const std::string& name);
    void setColor(color_t color);
    void setAniDir(AniDir aniDir);

    void setOwner(Tags* owner);

  public:
    Tags* m_owner;
    frame_t m_from, m_to;
    color_t m_color;
    std::string m_name;
    AniDir m_aniDir;

    // Disable operator=
    Tag& operator=(Tag&);
  };

} // namespace doc

#endif
