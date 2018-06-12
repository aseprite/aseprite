// Aseprite
// Copyright (C) 2018 David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TAGS_HANDLING_H_INCLUDED
#define APP_TAGS_HANDLING_H_INCLUDED
#pragma once

namespace app {

  // How to adjust tags when we move a frame in the border of a tag.
  enum TagsHandling {
    // Do not move tags.
    kDontAdjustTags,
    // Move tags "in the best possible way" when the user doesn't
    // specify what to do about them.
    kDefaultTagsAdjustment,
    // Put frames inside tags.
    kFitInsideTags,
    // Put frames outside tags.
    kFitOutsideTags,
  };

} // namespace app

#endif
