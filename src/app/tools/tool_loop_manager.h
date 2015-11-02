// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_TOOLS_TOOL_LOOP_MANAGER_H_INCLUDED
#define APP_TOOLS_TOOL_LOOP_MANAGER_H_INCLUDED
#pragma once

#include "app/tools/stroke.h"
#include "gfx/point.h"
#include "gfx/region.h"
#include "ui/keys.h"

#include <vector>

namespace gfx { class Region; }

namespace app {
  namespace tools {

    class ToolLoop;

    // Class to manage the drawing tool (editor <-> tool interface).
    //
    // The flow is this:
    // 1. The user press a mouse button in a Editor widget
    // 2. The Editor creates an implementation of ToolLoop and use it
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
    //
    class ToolLoopManager {
    public:

      // Simple container of mouse events information.
      class Pointer {
      public:
        enum Button { None, Left, Middle, Right };

        Pointer()
          : m_point(0, 0), m_button(None) { }

        Pointer(const gfx::Point& point, Button button)
          : m_point(point), m_button(button) { }

        const gfx::Point& point() const { return m_point; }
        Button getButton() const { return m_button; }

      private:
        gfx::Point m_point;
        Button m_button;
      };

      // Contructs a manager for the ToolLoop delegate.
      ToolLoopManager(ToolLoop* toolLoop);
      virtual ~ToolLoopManager();

      bool isCanceled() const;

      // Should be called when the user start a tool-trace (pressing the
      // left or right button for first time in the editor).
      void prepareLoop(const Pointer& pointer,
                       ui::KeyModifiers modifiers);

      void pressKey(ui::KeyScancode key);
      void releaseKey(ui::KeyScancode key);

      // Should be called each time the user presses a mouse button.
      void pressButton(const Pointer& pointer);

      // Should be called each time the user releases a mouse button.
      //
      // Returns true if the tool-loop should continue, or false
      // if the editor should release the mouse capture.
      bool releaseButton(const Pointer& pointer);

      // Should be called each time the user moves the mouse inside the editor.
      void movement(const Pointer& pointer);

    private:
      void doLoopStep(bool last_step);
      void snapToGrid(gfx::Point& point);

      void calculateDirtyArea(const Strokes& strokes);

      ToolLoop* m_toolLoop;
      Stroke m_stroke;
      Pointer m_lastPointer;
      gfx::Point m_oldPoint;
      gfx::Region& m_dirtyArea;
    };

  } // namespace tools
} // namespace app

#endif
