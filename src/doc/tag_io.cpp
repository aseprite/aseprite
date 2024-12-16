// Aseprite Document Library
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/tag_io.h"

#include "base/serialization.h"
#include "doc/string_io.h"
#include "doc/tag.h"
#include "doc/user_data_io.h"

#include <iostream>
#include <memory>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

void write_tag(std::ostream& os, const Tag* tag)
{
  std::string name = tag->name();

  write32(os, tag->id());
  write32(os, tag->fromFrame());
  write32(os, tag->toFrame());
  write8(os, (int)tag->aniDir());
  write_string(os, tag->name());
  write_user_data(os, tag->userData());
  write32(os, tag->repeat());
}

Tag* read_tag(std::istream& is, const bool setId, const SerialFormat serial)
{
  ObjectId id = read32(is);
  frame_t from = read32(is);
  frame_t to = read32(is);

  // If we are reading a session from v1.2.x, there is a color field
  color_t color;
  if (serial < SerialFormat::Ver1)
    color = read32(is);

  AniDir aniDir = (AniDir)read8(is);
  std::string name = read_string(is);
  UserData userData;

  // If we are reading the new v1.3.x version, there is a user data with the color + text
  int repeat = 0;
  if (serial >= SerialFormat::Ver1) {
    userData = read_user_data(is, serial);
    repeat = read32(is);
  }

  auto tag = std::make_unique<Tag>(from, to);
  tag->setAniDir(aniDir);
  tag->setName(name);
  if (serial < SerialFormat::Ver1)
    tag->setColor(color);
  else {
    tag->setUserData(userData);
    tag->setRepeat(repeat);
  }
  if (setId)
    tag->setId(id);
  return tag.release();
}

} // namespace doc
