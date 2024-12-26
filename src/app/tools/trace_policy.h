// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_TRACE_POLICY_H_INCLUDED
#define APP_TOOLS_TRACE_POLICY_H_INCLUDED
#pragma once

namespace app { namespace tools {

// The trace policy indicates how pixels are updated between the
// source image (ToolLoop::getSrcImage) and destionation image
// (ToolLoop::getDstImage) in the whole ToolLoopManager life-time.
// Basically it says if we should accumulate the intertwined
// drawed points, or kept the last ones, or overlap/composite
// them, etc.
enum class TracePolicy {

  // All pixels are accumulated in the destination image. Used by
  // freehand like tools.
  Accumulate,

  // Only the last trace is used. It means that on each ToolLoop
  // step, the destination image is completely invalidated and
  // restored from the source image. Used by
  // line/rectangle/ellipse-like tools.
  Last,

  // Like accumulate, but the destination is copied to the source
  // on each ToolLoop step, so the tool overlaps its own effect.
  // Used by jumble and spray.
  Overlap,

};

}} // namespace app::tools

#endif
