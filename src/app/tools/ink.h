// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_INK_H_INCLUDED
#define APP_TOOLS_INK_H_INCLUDED
#pragma once

#include "app/tools/stroke.h"

namespace gfx {
  class Region;
}

namespace app {
  namespace tools {

    class ToolLoop;

    // Class used to paint directly in the destination image (loop->getDstImage())
    //
    // The main task of this class is to draw scanlines through its
    // inkHline function member.
    class Ink {
    public:
      virtual ~Ink() { }

      // Creates a copy of the ink to avoid sharing state between
      // different ToolLoop implementations. (e.g. PaintInk::m_proc is
      // set in PaintInk::prepareInk() member function, so we cannot
      // share the same PaintInk instance.)
      virtual Ink* clone() = 0;

      // Returns true if this ink modifies the selection/mask
      virtual bool isSelection() const { return false; }

      // Returns true if this ink modifies the destination image
      virtual bool isPaint() const { return false; }

      // Returns true if this ink is an effect (is useful to know if a ink
      // is a effect so the Editor can display the cursor bounds)
      virtual bool isEffect() const { return false; }

      // Returns true if this ink acts like an eraser
      virtual bool isEraser() const { return false; }

      // Returns true if this ink picks colors from the image
      virtual bool isEyedropper() const { return false; }

      // Returns true if this ink is shading
      virtual bool isShading() const { return false; }

      // Returns true if this ink moves the scroll only
      virtual bool isScrollMovement() const { return false; }

      // Returns true if this ink is zoom
      virtual bool isZoom() const { return false; }

      // Returns true if this ink moves cels
      virtual bool isCelMovement() const { return false; }

      // Returns true if this ink selects layers automatically
      virtual bool isAutoSelectLayer() const { return false; }

      // Returns true if this ink is used to mark slices
      virtual bool isSlice() const { return false; }

      // Returns true if this tool uses the dithering options
      virtual bool withDitheringOptions() const { return false; }

      // Returns true if inkHline() needs source cel coordinates
      // instead of sprite coordinates (i.e. relative to
      // ToolLoop::getCelOrigin()).
      virtual bool needsCelCoordinates() const { return true; }

      // Returns true if this ink needs a special source area.  For
      // example, blur tool needs one extra pixel to all sides of the
      // modified area, so it can use a 3x3 convolution matrix.
      virtual bool needsSpecialSourceArea() const { return false; }
      virtual void createSpecialSourceArea(const gfx::Region& dirtyArea, gfx::Region& sourceArea) const { }

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

      // Called when we have to start using the ink for a new set of
      // strokes (e.g. color gradients is adjusted depending on the
      // first/last stroke points).
      virtual void prepareForStrokes(ToolLoop* loop, Strokes& strokes) { }

      // Called for each point shape.
      virtual void prepareForPointShape(ToolLoop* loop, bool firstPoint, int x, int y) { }
      virtual void prepareVForPointShape(ToolLoop* loop, int y) { }
      virtual void prepareUForPointShapeWholeScanline(ToolLoop* loop, int x1) { }
      virtual void prepareUForPointShapeSlicedScanline(ToolLoop* loop, bool leftSlice, int x1) { }

    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_INK_H_INCLUDED
