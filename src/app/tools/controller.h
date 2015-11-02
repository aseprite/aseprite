// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_CONTROLLER_H_INCLUDED
#define APP_TOOLS_CONTROLLER_H_INCLUDED
#pragma once

#include "gfx/point.h"
#include "ui/keys.h"

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

      virtual void prepareController(ui::KeyModifiers modifiers) { }

      // Called when the user presses or releases a key. Returns true
      // if the key is used (so a new mouse point is generated).
      virtual bool pressKey(ui::KeyScancode key) { return false; }
      virtual bool releaseKey(ui::KeyScancode key) { return false; }

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
      virtual void getStatusBarText(const Stroke& stroke, std::string& text) = 0;
    };

  } // namespace tools
} // namespace app

#endif  // APP_TOOLS_CONTROLLER_H_INCLUDED
