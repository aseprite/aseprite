// Aseprite Document Library
// Copyright (c) 2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_FRAME_RANGE_H_INCLUDED
#define DOC_FRAME_RANGE_H_INCLUDED
#pragma once

#include "doc/frame.h"

namespace doc {

struct FrameRange {
  frame_t fromFrame, toFrame;

  FrameRange() : fromFrame(0), toFrame(0) {}

  explicit FrameRange(frame_t frame) : fromFrame(frame), toFrame(frame) {}

  FrameRange(frame_t fromFrame, frame_t toFrame) : fromFrame(fromFrame), toFrame(toFrame) {}

  bool operator==(const FrameRange& o) const
  {
    return (fromFrame == o.fromFrame && toFrame == o.toFrame);
  }

  bool operator!=(const FrameRange& o) const { return !operator==(o); }
};

} // namespace doc

#endif
