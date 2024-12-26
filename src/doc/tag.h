// Aseprite Document Library
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_TAG_H_INCLUDED
#define DOC_TAG_H_INCLUDED
#pragma once

#include "doc/anidir.h"
#include "doc/frame.h"
#include "doc/object.h"
#include "doc/with_user_data.h"

#include <string>

namespace doc {
class Sprite;
class Tags;

class Tag : public WithUserData {
public:
  static constexpr int kMaxRepeat = 65535;

  Tag(frame_t from, frame_t to);
  Tag(const Tag& other);
  ~Tag();

  Sprite* sprite() const;
  Tags* owner() const { return m_owner; }
  frame_t fromFrame() const { return m_from; }
  frame_t toFrame() const { return m_to; }
  frame_t frames() const { return m_to - m_from + 1; }
  const std::string& name() const { return m_name; }
  color_t color() const { return userData().color(); }
  AniDir aniDir() const { return m_aniDir; }
  int repeat() const { return m_repeat; }

  void setFrameRange(frame_t from, frame_t to);
  void setName(const std::string& name);
  void setColor(color_t color);
  void setAniDir(AniDir aniDir);
  void setRepeat(int repeat);

  void setOwner(Tags* owner);

  bool contains(const frame_t frame) const { return (frame >= m_from && frame <= m_to); }

public:
  Tags* m_owner;
  frame_t m_from, m_to;
  std::string m_name;
  AniDir m_aniDir = AniDir::FORWARD;
  int m_repeat = 0;

  // Disable operator=
  Tag& operator=(Tag&);
};

} // namespace doc

#endif
