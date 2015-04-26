// Aseprite Render Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef RENDER_EXTRA_TYPE_H_INCLUDED
#define RENDER_EXTRA_TYPE_H_INCLUDED
#pragma once

namespace render {

  enum class ExtraType {
    NONE,

    // The extra cel indicates a "patch" for the current layer/frame
    // given in Render::setExtraImage()
    PATCH,

    // The extra cel indicates an extra composition for the current
    // layer/frame.
    COMPOSITE,
  };

} // namespace render

#endif
