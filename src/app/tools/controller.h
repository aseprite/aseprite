// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_CONTROLLER_H_INCLUDED
#define APP_TOOLS_CONTROLLER_H_INCLUDED
#pragma once

#include "app/tools/trace_policy.h"
#include "gfx/point.h"

#include <string>
#include <vector>

namespace app {
  namespace tools {

    class Stroke;
    class ToolLoop;

    // This class controls user input.
    class Controller {
    public:
      virtual ~Controller() { }

      virtual bool canSnapToGrid() { return true; }
      virtual bool isFreehand() { return false; }
      virtual bool isOnePoint() { return false; }
      virtual bool isTwoPoints() { return false; }

      virtual void prepareController(ToolLoop* loop) { }

      // Called when the user starts drawing and each time a new button is
      // pressed. The controller could be sure that this method is called
      // at least one time. The point is a position relative to sprite
      // bounds.
      virtual void pressButton(Stroke& stroke, const gfx::Point& point) = 0;

      // Called each time a mouse button is released.
      virtual bool releaseButton(Stroke& stroke, const gfx::Point& point) = 0;

      // Called when the mouse is moved.
      virtual void movement(ToolLoop* loop, Stroke& stroke, const gfx::Point& point) = 0;

      // The input and output strokes are relative to sprite coordinates.
      virtual void getStrokeToInterwine(const Stroke& input, Stroke& output) = 0;
      virtual void getStatusBarText(ToolLoop* loop, const Stroke& stroke, std::string& text) = 0;

      // Last point used by this controller, useful to save the last
      // point of a freehand tool.
      virtual gfx::Point getLastPoint() const { return gfx::Point(0, 0); }

      // Special trace policy that can change in the middle of the
      // ToolLoop. This is for LineFreehandController which uses a
      // TracePolicy::Last for TwoPoints controller (Line-like tool)
      // and then a TracePolicy::Accumulate for freehand (Pencil tool).
      virtual bool handleTracePolicy() const { return false; }
      virtual TracePolicy getTracePolicy() const { return TracePolicy::Accumulate; }

      // Returns the angle for a shape-like intertwiner (rectangles,
      // ellipses, etc.).
      virtual double getShapeAngle() const { return 0.0; }
    };

  } // namespace tools
} // namespace app

#endif  // APP_TOOLS_CONTROLLER_H_INCLUDED
