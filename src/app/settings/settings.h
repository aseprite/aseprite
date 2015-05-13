// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_SETTINGS_SETTINGS_H_INCLUDED
#define APP_SETTINGS_SETTINGS_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/settings/freehand_algorithm.h"
#include "app/settings/ink_type.h"
#include "app/settings/rotation_algorithm.h"
#include "app/settings/selection_mode.h"
#include "app/settings/right_click_mode.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "doc/brush_type.h"

namespace doc {
  class Document;
}

namespace app {

  class ColorSwatches;
  class IColorSwatchesStore;
  class IBrushSettings;
  class BrushSettingsObserver;
  class IToolSettings;
  class ToolSettingsObserver;
  class ISelectionSettings;
  class SelectionSettingsObserver;
  class GlobalSettingsObserver;

  namespace tools {
    class Tool;
  }

  class ISettings {
  public:
    virtual ~ISettings() { }

    // General settings
    virtual bool getShowSpriteEditorScrollbars() = 0;
    virtual RightClickMode getRightClickMode() = 0;
    virtual bool getGrabAlpha() = 0;
    virtual bool getAutoSelectLayer() = 0;
    virtual app::Color getFgColor() = 0;
    virtual app::Color getBgColor() = 0;
    virtual tools::Tool* getCurrentTool() = 0;
    virtual app::ColorSwatches* getColorSwatches() = 0;

    virtual void setShowSpriteEditorScrollbars(bool state) = 0;
    virtual void setRightClickMode(RightClickMode mode) = 0;
    virtual void setGrabAlpha(bool state) = 0;
    virtual void setAutoSelectLayer(bool state) = 0;
    virtual void setFgColor(const app::Color& color) = 0;
    virtual void setBgColor(const app::Color& color) = 0;
    virtual void setCurrentTool(tools::Tool* tool) = 0;
    virtual void setColorSwatches(app::ColorSwatches* colorSwatches) = 0;

    // Specific configuration for the given tool.
    virtual IToolSettings* getToolSettings(tools::Tool* tool) = 0;

    // Specific configuration for the current selection
    virtual ISelectionSettings* selection() = 0;

    virtual IColorSwatchesStore* getColorSwatchesStore() = 0;

    virtual void addObserver(GlobalSettingsObserver* observer) = 0;
    virtual void removeObserver(GlobalSettingsObserver* observer) = 0;
  };

  // Tool's settings
  class IToolSettings {
  public:
    virtual ~IToolSettings() { }

    virtual IBrushSettings* getBrush() = 0;

    virtual int getOpacity() = 0;
    virtual int getTolerance() = 0;
    virtual bool getContiguous() = 0;
    virtual bool getFilled() = 0;
    virtual bool getPreviewFilled() = 0;
    virtual int getSprayWidth() = 0;
    virtual int getSpraySpeed() = 0;
    virtual InkType getInkType() = 0;
    virtual FreehandAlgorithm getFreehandAlgorithm() = 0;

    virtual void setOpacity(int opacity) = 0;
    virtual void setTolerance(int tolerance) = 0;
    virtual void setContiguous(bool state) = 0;
    virtual void setFilled(bool state) = 0;
    virtual void setPreviewFilled(bool state) = 0;
    virtual void setSprayWidth(int width) = 0;
    virtual void setSpraySpeed(int speed) = 0;
    virtual void setInkType(InkType inkType) = 0;
    virtual void setFreehandAlgorithm(FreehandAlgorithm algorithm) = 0;

    virtual void addObserver(ToolSettingsObserver* observer) = 0;
    virtual void removeObserver(ToolSettingsObserver* observer) = 0;
  };

  // Settings for a tool's brush
  class IBrushSettings {
  public:
    virtual ~IBrushSettings() { }

    virtual doc::BrushType getType() = 0;
    virtual int getSize() = 0;
    virtual int getAngle() = 0;

    virtual void setType(BrushType type) = 0;
    virtual void setSize(int size) = 0;
    virtual void setAngle(int angle) = 0;

    virtual void addObserver(BrushSettingsObserver* observer) = 0;
    virtual void removeObserver(BrushSettingsObserver* observer) = 0;
  };

  class ISelectionSettings {
  public:
    virtual ~ISelectionSettings() {}

    // Mask color used during a move operation
    virtual SelectionMode getSelectionMode() = 0;
    virtual app::Color getMoveTransparentColor() = 0;
    virtual RotationAlgorithm getRotationAlgorithm() = 0;

    virtual void setSelectionMode(SelectionMode mode) = 0;
    virtual void setMoveTransparentColor(app::Color color) = 0;
    virtual void setRotationAlgorithm(RotationAlgorithm algorithm) = 0;

    virtual void addObserver(SelectionSettingsObserver* observer) = 0;
    virtual void removeObserver(SelectionSettingsObserver* observer) = 0;
  };

  class IColorSwatchesStore {
  public:
    virtual ~IColorSwatchesStore() { }
    virtual void addColorSwatches(app::ColorSwatches* colorSwatches) = 0;
    virtual void removeColorSwatches(app::ColorSwatches* colorSwatches) = 0;
  };

} // namespace app

#endif
