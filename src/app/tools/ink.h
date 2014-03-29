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

#ifndef APP_TOOLS_INK_H_INCLUDED
#define APP_TOOLS_INK_H_INCLUDED
#pragma once

namespace app {
  namespace tools {

    class ToolLoop;

    // Class used to paint directly in the destination image (loop->getDstImage())
    //
    // The main task of this class is to draw scanlines through its
    // inkHline function member.
    class Ink {
      // selection, paint, paint_fg, paint_bg, eraser,
      // replace_fg_with_bg, replace_bg_with_fg, pick_fg, pick_bg, scroll,
      // move, shade, blur, jumble
    public:
      virtual ~Ink() { }

      // Returns true if this ink modifies the selection/mask
      virtual bool isSelection() const { return false; }

      // Returns true if this ink modifies the destination image
      virtual bool isPaint() const { return false; }

      // Returns true if this ink is an effect (is useful to know if a ink
      // is a effect so the Editor can display the cursor bounds)
      virtual bool isEffect() const { return false; }

      // Returns true if this ink picks colors from the image
      virtual bool isEyedropper() const { return false; }

      // Returns true if this ink moves the scroll only
      virtual bool isScrollMovement() const { return false; }

      // Returns true if this ink moves cels
      virtual bool isCelMovement() const { return false; }

      // It is called when the tool-loop start (generally when the user
      // presses a mouse button over a sprite editor)
      virtual void prepareInk(ToolLoop* loop) { }

      // It is used in the final stage of the tool-loop, it is called twice
      // (first with state=true and then state=false)
      virtual void setFinalStep(ToolLoop* loop, bool state) { }

      // It is used to paint scanlines in the destination image.
      // PointShapes call this method when they convert a mouse-point
      // to a shape (e.g. pen shape)  with various scanlines.
      virtual void inkHline(int x1, int y, int x2, ToolLoop* loop) = 0;

    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_INK_H_INCLUDED
