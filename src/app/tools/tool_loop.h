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

#ifndef APP_TOOLS_TOOL_LOOP_H_INCLUDED
#define APP_TOOLS_TOOL_LOOP_H_INCLUDED

#include "app/tools/trace_policy.h"
#include "filters/tiled_mode.h"
#include "gfx/point.h"

namespace gfx {
  class Region;
}

namespace raster {
  class Image;
  class Layer;
  class Mask;
  class Pen;
  class RgbMap;
  class Sprite;
}

namespace app {
  class Context;
  class Document;
  class IDocumentSettings;
  class ISettings;

  namespace tools {
    class Controller;
    class Ink;
    class Intertwine;
    class PointShape;
    class ShadingOptions;
    class Tool;

    using namespace raster;

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

      // Returns the tool to use to draw or use
      virtual Tool* getTool() = 0;

      // Returns the pen which will be used with the tool
      virtual Pen* getPen() = 0;

      // Returns the document to which belongs the sprite.
      virtual Document* getDocument() = 0;

      // Returns the sprite where we will draw on
      virtual Sprite* getSprite() = 0;

      // Returns the layer that will be modified if the tool paints
      virtual Layer* getLayer() = 0;

      // Should return an image where we can read pixels (readonly image)
      virtual Image* getSrcImage() = 0;

      // Should return an image where we can write pixels
      virtual Image* getDstImage() = 0;

      // Returns the RGB map used to convert RGB values to palette index.
      virtual RgbMap* getRgbMap() = 0;

      // Returns true if we should use the mask to limit the paint area.
      virtual bool useMask() = 0;

      // Current mask to limit paint area
      virtual Mask* getMask() = 0;

      // Gets mask X,Y origin coordinates
      virtual gfx::Point getMaskOrigin() = 0;

      // Return the mouse button which start the tool-loop. It can be used
      // by some tools that instead of using the primary/secondary color
      // uses the pressed button for different behavior (like selection
      // tools).
      virtual Button getMouseButton() = 0;

      // Primary color to draw (e.g. foreground if the user start drawing
      // with the left button, or background color if he used the right
      // button)
      virtual int getPrimaryColor() = 0;
      virtual void setPrimaryColor(int color) = 0;

      // Secondary color to draw (e.g. background if the user start drawing
      // with the left button, or foreground color if he used the right
      // button)
      virtual int getSecondaryColor() = 0;
      virtual void setSecondaryColor(int color) = 0;

      // Returns the opacity to be used by the ink (Ink).
      virtual int getOpacity() = 0;

      // Returns the tolerance to be used by the ink (Ink).
      virtual int getTolerance() = 0;

      // Returns the current settings. Used to know current
      // foreground/background color (certain tools needs to know the
      // exact foreground/background color, they cannot used the
      // primary/secondary).
      virtual ISettings* getSettings() = 0;

      // Returns the document settings (tiled mode, grid bounds, etc.).
      // It's used to know the preferred "tiled" mode of the document.
      // See the method PointShape::doInkHline to check how this member is
      // used. When tiled mode is activated, each scanline can be divided
      // in various sub-lines if they pass the image bounds. For each of
      // these scanlines a Ink::inkHline is called
      // Also it's used to know the grid/snap-to-grid settings/behavior
      // (see ToolLoopManager::snapToGrid).
      virtual IDocumentSettings* getDocumentSettings() = 0;

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

      // Offset for each point
      virtual gfx::Point getOffset() = 0;

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

      virtual ShadingOptions* getShadingOptions() = 0;

      // Used by the tool when the user cancels the operation pressing the
      // other mouse button.
      virtual void cancel() = 0;

      // Returns true if the loop was canceled by the user
      virtual bool isCanceled() = 0;

      // Converts a coordinate in the screen to the sprite.
      virtual gfx::Point screenToSprite(const gfx::Point& screenPoint) = 0;

      // This region is modified by the ToolLoopManager so then you know
      // what must be updated in updateDirtyArea().
      virtual gfx::Region& getDirtyArea() = 0;

      // Redraws the dirty area.
      virtual void updateDirtyArea() = 0;

      virtual void updateStatusBar(const char* text) = 0;

    };

  } // namespace tools
} // namespace app

#endif  // TOOLS_TOOL_LOOP_H_INCLUDED
