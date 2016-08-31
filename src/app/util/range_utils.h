// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UTIL_RANGE_UTILS_H_INCLUDED
#define APP_UTIL_RANGE_UTILS_H_INCLUDED
#pragma once

#include "doc/cel_list.h"

#include <vector>

namespace doc {
  class Sprite;
}

namespace app {
  using namespace doc;

  class DocumentRange;

  doc::CelList get_unique_cels(doc::Sprite* sprite, const DocumentRange& range);

} // namespace app

#endif
