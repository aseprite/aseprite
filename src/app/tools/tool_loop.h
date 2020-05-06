// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_TOOL_LOOP_H_INCLUDED
#define APP_TOOLS_TOOL_LOOP_H_INCLUDED
#pragma once

#include "app/shade.h"
#include "app/tools/dynamics.h"
#include "app/tools/tool_loop_modifiers.h"
#include "app/tools/trace_policy.h"
#include "doc/brush.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "filters/tiled_mode.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "render/gradient.h"

namespace gfx {
  class Region;
}

namespace doc {
  class Image;
  class Layer;
  class Mask;
  class Remap;
  class RgbMap;
  class Slice;
  class Sprite;
}

namespace render {
  class DitheringAlgorithmBase;
  class DitheringMatrix;
}

namespace app {
  class Context;
  class Doc;

  namespace tools {
    class Controller;
    class Ink;
    class Intertwine;
    class PointShape;
    class Symmetry;
    class Tool;

    using namespace doc;

    // Interface to communicate the sprite editor with the tool when the user
    // starts using a tool to paint, select, pick color, etc.
    //
    // All this information should be provided by the editor and consumed
    // by the tool (+controller+intertwiner+pointshape+ink).
    //
    // TODO This interface is huge, it should be refactored.
    class ToolLoop {
    public:
      enum Button { Left = 0, Right = 1 };

      virtual ~ToolLoop() { }

      virtual void commitOrRollback() = 0;

      // Returns the tool to use to draw or use
      virtual Tool* getTool() = 0;

      // Returns the brush which will be used with the tool
      virtual Brush* getBrush() = 0;
      virtual void setBrush(const BrushRef& newBrush) = 0;

      // Returns the document to which belongs the sprite.
      virtual Doc* getDocument() = 0;

      // Returns the sprite where we will draw on
      virtual Sprite* sprite() = 0;

      // Returns the layer that will be modified if the tool paints
      virtual Layer* getLayer() = 0;

      // Returns the frame where we're paiting
      virtual frame_t getFrame() = 0;

      // Should return an image where we can read pixels (readonly image)
      virtual const Image* getSrcImage() = 0;

      // The image used to get get pixels in floodfill algorithm.
      virtual const Image* getFloodFillSrcImage() = 0;

      // Should return an image where we can write pixels
      virtual Image* getDstImage() = 0;

      // Makes the specified region valid in the source
      // image. Basically the implementation should copy from the
      // original cel the given region to the source image. The source
      // image is used by inks to create blur effects or similar.
      virtual void validateSrcImage(const gfx::Region& rgn) = 0;

      // Makes the specified destination image region valid to be
      // painted. The destination image is used by inks to compose the
      // brush, so we've to make sure that the destination image
      // matches the original cel when we make that composition.
      virtual void validateDstImage(const gfx::Region& rgn) = 0;

      // Invalidates the whole destination image. It's used for tools
      // like line or rectangle which don't accumulate the effect so
      // they need to start with a fresh destination image on each
      // loop step/cycle.
      virtual void invalidateDstImage() = 0;
      virtual void invalidateDstImage(const gfx::Region& rgn) = 0;

      // Copies the given region from the destination to the source
      // image, used by "overlap" tools like jumble or spray.
      virtual void copyValidDstToSrcImage(const gfx::Region& rgn) = 0;

      // Returns the RGB map used to convert RGB values to palette index.
      virtual RgbMap* getRgbMap() = 0;

      // Returns true if we should use the mask to limit the paint area.
      virtual bool useMask() = 0;

      // Current mask to limit paint area
      virtual Mask* getMask() = 0;
      virtual void setMask(Mask* newMask) = 0;

      // Gets mask X,Y origin coordinates
      virtual gfx::Point getMaskOrigin() = 0;

      // Return the mouse button which start the tool-loop. It can be used
      // by some tools that instead of using the primary/secondary color
      // uses the pressed button for different behavior (like selection
      // tools).
      virtual Button getMouseButton() = 0;

      // Returns active foreground/background color (certain tools
      // needs to know the exact foreground/background color, they
      // cannot used the primary/secondary).
      virtual doc::color_t getFgColor() = 0;
      virtual doc::color_t getBgColor() = 0;

      // Primary color to draw (e.g. foreground if the user start drawing
      // with the left button, or background color if he used the right
      // button)
      virtual doc::color_t getPrimaryColor() = 0;
      virtual void setPrimaryColor(doc::color_t color) = 0;

      // Secondary color to draw (e.g. background if the user start drawing
      // with the left button, or foreground color if he used the right
      // button)
      virtual doc::color_t getSecondaryColor() = 0;
      virtual void setSecondaryColor(doc::color_t color) = 0;

      // Returns the opacity to be used by the ink (Ink).
      virtual int getOpacity() = 0;

      // Returns the tolerance to be used by the ink (Ink).
      virtual int getTolerance() = 0;

      // Returns true if the flood fill algorithm should take care
      // contiguous pixels or not.
      virtual bool getContiguous() = 0;

      // Returns flags/modifiers that change the way each part of the
      // tool (ink/controllers/etc.) work.
      virtual tools::ToolLoopModifiers getModifiers() = 0;

      // Returns the preferred "tiled" mode of the document.
      // See the method PointShape::doInkHline to check how this member is
      // used. When tiled mode is activated, each scanline can be divided
      // in various sub-lines if they pass the image bounds. For each of
      // these scanlines a Ink::inkHline is called
      virtual filters::TiledMode getTiledMode() = 0;

      virtual bool getGridVisible() = 0;
      virtual bool getSnapToGrid() = 0;
      virtual bool isSelectingTiles() = 0;
      virtual bool getStopAtGrid() = 0; // For floodfill-like tools
      virtual gfx::Rect getGridBounds() = 0;
      virtual bool isPixelConnectivityEightConnected() = 0;

      // Returns true if the figure must be filled when we release the
      // mouse (e.g. a filled rectangle, etc.)
      //
      // To fill a shape, the Intertwine::fillPoints function is used.
      virtual bool getFilled() = 0;

      // Returns true if the preview should be with filled shapes.
      virtual bool getPreviewFilled() = 0;

      // Spray configuration
      virtual int getSprayWidth() = 0;
      virtual int getSpraySpeed() = 0;

      // X,Y origin of the cel where we are drawing
      virtual gfx::Point getCelOrigin() = 0;

      // Velocity vector of the mouse
      virtual void setSpeed(const gfx::Point& speed) = 0;
      virtual gfx::Point getSpeed() = 0;

      // Returns the ink to use with the tool. Each tool has an associated
      // ink, but it could be modified for this specific loop, so
      // generally you should return the same ink as the tool, but it can
      // be different. The same for the other properties.
      virtual Ink* getInk() = 0;
      virtual Controller* getController() = 0;
      virtual PointShape* getPointShape() = 0;
      virtual Intertwine* getIntertwine() = 0;
      virtual TracePolicy getTracePolicy() = 0;
      virtual Symmetry* getSymmetry() = 0;

      virtual const Shade& getShade() = 0;
      virtual const doc::Remap* getShadingRemap() = 0;

      // Used by the tool when the user cancels the operation pressing the
      // other mouse button.
      virtual void cancel() = 0;

      // Returns true if the loop was canceled by the user
      virtual bool isCanceled() = 0;

      virtual void limitDirtyAreaToViewport(gfx::Region& rgn) = 0;

      // Redraws the dirty area.
      virtual void updateDirtyArea(const gfx::Region& dirtyArea) = 0;

      virtual void updateStatusBar(const char* text) = 0;
      virtual gfx::Point statusBarPositionOffset() = 0;

      // For gradients
      virtual render::DitheringMatrix getDitheringMatrix() = 0;
      virtual render::DitheringAlgorithmBase* getDitheringAlgorithm() = 0;
      virtual render::GradientType getGradientType() = 0;

      // For freehand algorithms with dynamics
      virtual tools::DynamicsOptions getDynamics() = 0;

      // Called when the user release the mouse on SliceInk
      virtual void onSliceRect(const gfx::Rect& bounds) = 0;
    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_TOOL_LOOP_H_INCLUDED
