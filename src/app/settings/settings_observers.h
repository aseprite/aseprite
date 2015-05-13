// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
#include "doc/brush_type.h"

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
    virtual void onSetBrushType(doc::BrushType newType) {}
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

} // namespace app

#endif // APP_SETTINGS_SETTINGS_OBSERVERS_H_INCLUDED
