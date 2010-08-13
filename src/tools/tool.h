/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#ifndef TOOLS_TOOL_H_INCLUDED
#define TOOLS_TOOL_H_INCLUDED

#include <string>
#include <list>
#include <vector>

#include "Vaca/Point.h"
#include "Vaca/Rect.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"

#include "tiled_mode.h"

using Vaca::Point;
using Vaca::Rect;

class Context;
class Sprite;
class Image;
class Layer;
class Mask;
class Tool;
class Pen;

class IToolLoop;

enum ToolFill
{
  TOOL_FILL_NONE,
  TOOL_FILL_ALWAYS,
  TOOL_FILL_OPTIONAL,
};

enum ToolTracePolicy
{
  TOOL_TRACE_POLICY_ACCUMULATE,
  TOOL_TRACE_POLICY_LAST,
};

// Class used to paint directly in the destination image (loop->getDstImage())
// 
// The main task of this class is to draw scanlines through its
// inkHline function member.
class ToolInk
{
  // selection, paint, paint_fg, paint_bg, eraser,
  // replace_fg_with_bg, replace_bg_with_fg, pick_fg, pick_bg, scroll,
  // move, shade, blur, jumble
public:
  virtual ~ToolInk() { }

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
  virtual void prepareInk(IToolLoop* loop) { }

  // It is used in the final stage of the tool-loop, it is called twice
  // (first with state=true and then state=false)
  virtual void setFinalStep(IToolLoop* loop, bool state) { }

  // It is used to paint scanlines in the destination image.
  // ToolPointShapes call this method when they convert a mouse-point
  // to a shape (e.g. pen shape)  with various scanlines.
  virtual void inkHline(int x1, int y, int x2, IToolLoop* loop) = 0;

};

// This class controls user input.
class ToolController
{
  // freehand, point_by_point, one_point, two_points, four_points
public:
  virtual ~ToolController() { }
  virtual bool canSnapToGrid() { return true; }
  virtual void pressButton(std::vector<Point>& points, const Point& point) = 0;
  virtual bool releaseButton(std::vector<Point>& points, const Point& point) = 0;
  virtual void movement(IToolLoop* loop, std::vector<Point>& points, const Point& point) = 0;
  virtual void getPointsToInterwine(const std::vector<Point>& input, std::vector<Point>& output) = 0;
  virtual void getStatusBarText(const std::vector<Point>& points, std::string& text) = 0;
};

// Converts a point to a shape to be drawn
class ToolPointShape
{
  // none, pixel, pen, floodfill, spray
public:
  virtual ~ToolPointShape() { }
  virtual bool isFloodFill() { return false; }
  virtual bool isSpray() { return false; }
  virtual void transformPoint(IToolLoop* loop, int x, int y) = 0;
  virtual void getModifiedArea(IToolLoop* loop, int x, int y, Rect& area) = 0;

protected:
  // Calls loop->getInk()->inkHline() function for each horizontal-scanline
  // that should be drawn (applying the "tiled" mode loop->getTiledMode())
  static void doInkHline(int x1, int y, int x2, IToolLoop* loop);
};

class ToolIntertwine
{
  // none, as_lines, as_bezier, as_rectangles, as_ellipses
public:
  virtual ~ToolIntertwine() { }
  virtual bool snapByAngle() { return false; }
  virtual void joinPoints(IToolLoop* loop, const std::vector<Point>& points) = 0;
  virtual void fillPoints(IToolLoop* loop, const std::vector<Point>& points) = 0;

protected:
  static void doPointshapePoint(int x, int y, IToolLoop* loop);
  static void doPointshapeHline(int x1, int y, int x2, IToolLoop* loop);
  static void doPointshapeLine(int x1, int y1, int x2, int y2, IToolLoop* loop);
};

// A group of tools.
class ToolGroup
{
  std::string m_name;
  std::string m_label;
public:
  ToolGroup(const char* name,
	    const char* label) : m_name(name)
			       , m_label(label) { }

  const std::string& getName() const { return m_name; }
  const std::string& getLabel() const { return m_label; }
};

// A drawing tool
class Tool
{
  ToolGroup* m_group;
  std::string m_id;
  std::string m_text;
  std::string m_tips;
  int m_default_pen_size;

  struct {
    ToolFill m_fill;
    ToolInk* m_ink;
    ToolController* m_controller;
    ToolPointShape* m_point_shape;
    ToolIntertwine* m_intertwine;
    ToolTracePolicy m_trace_policy;
  } m_button[2]; // Two buttons: [0] left and [1] right

public:

  Tool(ToolGroup* group,
       const std::string& id,
       const std::string& text,
       const std::string& tips,
       int default_pen_size)
    : m_group(group)
    , m_id(id)
    , m_text(text)
    , m_tips(tips)
    , m_default_pen_size(default_pen_size)
  { }

  virtual ~Tool()
  { }

  const ToolGroup* getGroup() const { return m_group; }
  const std::string& getId() const { return m_id; }
  const std::string& getText() const { return m_text; }
  const std::string& getTips() const { return m_tips; }
  int getDefaultPenSize() const { return m_default_pen_size; }

  ToolFill getFill(int button) { return m_button[button].m_fill; }
  ToolInk* getInk(int button) { return m_button[button].m_ink; }
  ToolController* getController(int button) { return m_button[button].m_controller; }
  ToolPointShape* getPointShape(int button) { return m_button[button].m_point_shape; }
  ToolIntertwine* getIntertwine(int button) { return m_button[button].m_intertwine; }
  ToolTracePolicy getTracePolicy(int button) { return m_button[button].m_trace_policy; }

  void setFill(int button, ToolFill fill) { m_button[button].m_fill = fill; }
  void setInk(int button, ToolInk* ink) { m_button[button].m_ink = ink; }
  void setController(int button, ToolController* controller) { m_button[button].m_controller = controller; }
  void setPointShape(int button, ToolPointShape* point_shape) { m_button[button].m_point_shape = point_shape; }
  void setIntertwine(int button, ToolIntertwine* intertwine) { m_button[button].m_intertwine = intertwine; }
  void setTracePolicy(int button, ToolTracePolicy trace_policy) { m_button[button].m_trace_policy = trace_policy; }

};

// Interface to communicate the sprite editor with the tool when the user
// start using a tool to paint, select, pick color, etc.
//
// All this information should be provided by the editor and consumed
// by the tool (+controller+intertwiner+pointshape+ink).
class IToolLoop
{
public:
  virtual ~IToolLoop() { }

  // Returns the context where we want to draw on (generally UIContext::instance() singleton)
  virtual Context* getContext() = 0;

  // Returns the tool to use to draw or use
  virtual Tool* getTool() = 0;

  // Returns the pen which will be used with the tool
  virtual Pen* getPen() = 0;

  // Returns the sprite where we will draw on
  virtual Sprite* getSprite() = 0;

  // Returns the layer that will be modified if the tool paints
  virtual Layer* getLayer() = 0;

  // Should return an image where we can read pixels (readonly image)
  virtual Image* getSrcImage() = 0;

  // Should return an image where we can write pixels
  virtual Image* getDstImage() = 0;

  // Current mask to limit paint area
  virtual Mask* getMask() = 0;

  // Gets mask X,Y origin coordinates
  virtual Point getMaskOrigin() = 0;

  // Return the mouse button which start the tool-loop (0 = left
  // button, 1 = right button). It can be used by some tools that
  // instead of using the primary/secondary color uses the pressed
  // button for different behavior (like selection tools)
  virtual int getMouseButton() = 0;

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

  // Returns the opacity to be used by the ink (ToolInk).
  virtual int getOpacity() = 0;

  // Returns the tolerance to be used by the ink (ToolInk).
  virtual int getTolerance() = 0;

  // Returns true if each scanline generated by a ToolPointShape must
  // be "tiled".  See the method ToolPointShape::doInkHline to check
  // how this member is used. When tiled mode is activated, each
  // scanline can be divided in various sub-lines if they pass the
  // image bounds. For each of these scanlines a ToolInk::inkHline
  // is called
  virtual TiledMode getTiledMode() = 0;

  // Returns true if the figure must be filled when we release the
  // mouse (e.g. a filled rectangle, etc.)
  // 
  // To fill a shape, the ToolIntertwine::fillPoints function is used.
  virtual bool getFilled() = 0;

  // Returns true if the preview should be with filled shapes.
  virtual bool getPreviewFilled() = 0;

  // Spray configuration
  virtual int getSprayWidth() = 0;
  virtual int getSpraySpeed() = 0;

  // Offset for each point
  virtual Point getOffset() = 0;

  // Velocity vector of the mouse
  virtual void setSpeed(const Point& speed) = 0;
  virtual Point getSpeed() = 0;

  // Returns the ink to use with the tool. Each tool has an associated
  // ink, but it could be modified for this specific loop, so
  // generally you should return the same ink as the tool, but it can
  // be different. The same for the other properties.
  virtual ToolInk* getInk() = 0;
  virtual ToolController* getController() = 0;
  virtual ToolPointShape* getPointShape() = 0;
  virtual ToolIntertwine* getIntertwine() = 0;
  virtual ToolTracePolicy getTracePolicy() = 0;

  // Used by the tool when the user cancels the operation pressing the
  // other mouse button.
  virtual void cancel() = 0;

  // Returns true if the loop was canceled by the user
  virtual bool isCanceled() = 0;

  // Converts a coordinate in the screen to the sprite.
  virtual Point screenToSprite(const Point& screenPoint) = 0;

  // Redraws in the screen the specified are of sprite.
  virtual void updateArea(const Rect& dirty_area) = 0;

  virtual void updateStatusBar(const char* text) = 0;

};

// Class to manage the drawing tool (editor <-> tool interface).
// 
// The flow is this:
// 1. The user press a mouse button in a Editor widget
// 2. The Editor creates an implementation of IToolLoop and use it 
//    with the ToolLoopManager constructor
// 3. The ToolLoopManager is used to call
//    the following methods:
//    - ToolLoopManager::prepareLoop
//    - ToolLoopManager::pressButton
// 4. If the user moves the mouse, the method
//    - ToolLoopManager::movement
//    is called.
// 5. When the user release the mouse:
//    - ToolLoopManager::releaseButton
//    - ToolLoopManager::releaseLoop
class ToolLoopManager
{
  IToolLoop* m_toolLoop;
  std::vector<Point> m_points;
  Point m_oldPoint;
  
public:

  ToolLoopManager(IToolLoop* toolLoop);
  virtual ~ToolLoopManager();

  bool isCanceled() const { return m_toolLoop->isCanceled(); }

  // Should be called when the user start a tool-trace (pressing the
  // left or right button for first time in the editor).
  void prepareLoop(JMessage msg);

  // Called when the loop is over.
  void releaseLoop(JMessage msg);

  // Should be called each time the user presses a mouse button.
  void pressButton(JMessage msg);

  // Should be called each time the user releases a mouse button.
  //
  // Returns true if the tool-loop should continue, or false
  // if the editor should release the mouse capture.
  bool releaseButton(JMessage msg);

  // Should be called each time the user moves the mouse inside the editor.
  void movement(JMessage msg);

private:
  void doLoopStep(bool last_step);
  void snapToGrid(bool flexible, Point& point);

  static void calculateDirtyArea(IToolLoop* loop, const std::vector<Point>& points, Rect& dirty_area);
  static void calculateMinMax(const std::vector<Point>& points, Point& minpt, Point& maxpt);
};

#endif

