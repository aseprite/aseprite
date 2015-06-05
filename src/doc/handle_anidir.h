// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_HANDLE_ANIDIR_H_INCLUDED
#define DOC_HANDLE_ANIDIR_H_INCLUDED
#pragma once

#include "doc/frame.h"

namespace doc {

  class FrameTag;
  class Sprite;

  frame_t calculate_next_frame(
    const Sprite* sprite,
    frame_t frame,
    frame_t frameDelta,
    const FrameTag* tag,
    bool& pingPongForward);

} // namespace doc

#endif
