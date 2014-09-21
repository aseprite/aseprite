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

#ifndef APP_UI_EDITOR_TRANSFORM_HANDLES_H_INCLUDED
#define APP_UI_EDITOR_TRANSFORM_HANDLES_H_INCLUDED
#pragma once

#include "app/ui/editor/handle_type.h"
#include "fixmath/fixmath.h"
#include "gfx/point.h"
#include "gfx/transformation.h"

namespace ui {
  class Graphics;
}

namespace app {
  class Editor;

  // Helper class to do hit-detection and render transformation handles
  // and rotation pivot.
  class TransformHandles {
  public:
    TransformHandles();
    ~TransformHandles();

    // Returns the handle in the given mouse point (pt) when the user
    // has applied the given transformation to the selection.
    HandleType getHandleAtPoint(Editor* editor, const gfx::Point& pt, const gfx::Transformation& transform);

    void drawHandles(Editor* editor, const gfx::Transformation& transform);
    void invalidateHandles(Editor* editor, const gfx::Transformation& transform);

  private:
    gfx::Rect getPivotHandleBounds(Editor* editor,
      const gfx::Transformation& transform,
      const gfx::Transformation::Corners& corners);

    bool inHandle(const gfx::Point& pt, int x, int y, int gfx_w, int gfx_h, fixmath::fixed angle);
    void drawHandle(ui::Graphics* g, int x, int y, fixmath::fixed angle);
    void adjustHandle(int& x, int& y, int handle_w, int handle_h, fixmath::fixed angle);
  };

} // namespace app

#endif
