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

#ifndef APP_SETTINGS_SETTINGS_OBSERVERS_H_INCLUDED
#define APP_SETTINGS_SETTINGS_OBSERVERS_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/settings/freehand_algorithm.h"
#include "app/settings/ink_type.h"
#include "app/settings/rotation_algorithm.h"
#include "app/settings/selection_mode.h"
#include "filters/tiled_mode.h"
#include "gfx/fwd.h"
#include "raster/brush_type.h"

namespace app {
  class Color;

  namespace tools {
    class Tool;
  }
  class ColorSwatches;

  class BrushSettingsObserver {
  public:
    virtual ~BrushSettingsObserver() {}

    virtual void onSetBrushSize(int newSize) {}
    virtual void onSetBrushType(raster::BrushType newType) {}
    virtual void onSetBrushAngle(int newAngle) {}
  };

  class ToolSettingsObserver {
  public:
    virtual ~ToolSettingsObserver() {}

    virtual void onSetOpacity(int newOpacity) {}
    virtual void onSetTolerance(int newTolerance) {}
    virtual void onSetFilled(bool filled) {}
    virtual void onSetPreviewFilled(bool previewFilled) {}
    virtual void onSetSprayWidth(int newSprayWidth) {}
    virtual void onSetSpraySpeed(int newSpraySpeed) {}
    virtual void onSetInkType(InkType newInkType) {}
    virtual void onSetFreehandAlgorithm(FreehandAlgorithm algorithm) {}
  };

  class SelectionSettingsObserver {
  public:
    virtual ~SelectionSettingsObserver() {}

    virtual void onSetSelectionMode(SelectionMode mode) {}
    virtual void onSetMoveTransparentColor(app::Color newColor) {}
    virtual void onSetRotationAlgorithm(RotationAlgorithm algorithm) {}
  };

  class GlobalSettingsObserver {
  public:
    virtual ~GlobalSettingsObserver() {}

    virtual void onSetShowSpriteEditorScrollbars(bool state) {}
    virtual void onSetGrabAlpha(bool state) {}
    virtual void onSetFgColor(app::Color newColor) {}
    virtual void onSetBgColor(app::Color newColor) {}
    virtual void onSetCurrentTool(tools::Tool* newTool) {}
    virtual void onSetColorSwatches(ColorSwatches* swaches) {}
  };

  class DocumentSettingsObserver {
  public:
    virtual ~DocumentSettingsObserver() { }

    virtual void onSetTiledMode(filters::TiledMode mode) { }
    virtual void onSetSnapToGrid(bool state) { }
    virtual void onSetGridVisible(bool state) { }
    virtual void onSetGridBounds(const gfx::Rect& rect) { }
    virtual void onSetGridColor(const app::Color& color) { }
  };

} // namespace app

#endif // APP_SETTINGS_SETTINGS_OBSERVERS_H_INCLUDED
