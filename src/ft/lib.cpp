// Aseprite FreeType Wrapper
// Copyright (c) 2016-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "ft/lib.h"

#include "base/log.h"
#include "ft/stream.h"

#include <iostream>

namespace ft {

Lib::Lib()
  : m_ft(nullptr)
{
  FT_Init_FreeType(&m_ft);
}

Lib::~Lib()
{
  if (m_ft)
    FT_Done_FreeType(m_ft);
}

FT_Face Lib::open(const std::string& filename)
{
  FT_Stream stream = ft::open_stream(m_ft, filename);
  FT_Open_Args args;
  memset(&args, 0, sizeof(args));
  args.flags = FT_OPEN_STREAM;
  args.stream = stream;

  LOG(VERBOSE) << "FT: Loading font '" << filename << "'\n";

  FT_Face face = nullptr;
  FT_Error err = FT_Open_Face(m_ft, &args, 0, &face);
  if (!err)
    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
  return face;
}

} // namespace ft
