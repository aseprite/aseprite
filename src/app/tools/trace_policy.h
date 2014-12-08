/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_TOOLS_TRACE_POLICY_H_INCLUDED
#define APP_TOOLS_TRACE_POLICY_H_INCLUDED
#pragma once

namespace app {
  namespace tools {

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

      // It's like accumulate, but the last modified area in the
      // destination is invalidated and redraw from the source image +
      // tool trace. It's used by pixel-perfect freehand algorithm
      // (because last modified pixels can differ).
      AccumulateUpdateLast,

      // Only the last trace is used. It means that on each ToolLoop
      // step, the destination image is completely invalidated and
      // restored from the source image. Used by
      // line/rectangle/ellipse-like tools.
      Last,

      // Like accumulate, but the destination is copied to the source
      // on each ToolLoop step, so the tool overlaps its own effect.
      // Used by blur, jumble, or spray.
      Overlap,

    };

  } // namespace tools
} // namespace app

#endif
