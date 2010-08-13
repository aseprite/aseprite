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

#ifndef SETTINGS_SETTINGS_H_INCLUDED
#define SETTINGS_SETTINGS_H_INCLUDED

#include "Vaca/Rect.h"
#include "core/color.h"
#include "tiled_mode.h"
#include "pen_type.h"

using Vaca::Rect;

class IToolSettings;
class IPenSettings;
class Tool;

// Settings used in tool <-> drawing <-> editor stuff
class ISettings
{
public:
  virtual ~ISettings() { }

  // General settings

  virtual color_t getFgColor() = 0;
  virtual color_t getBgColor() = 0;
  virtual Tool* getCurrentTool() = 0;
  virtual TiledMode getTiledMode() = 0;

  virtual void setFgColor(color_t color) = 0;
  virtual void setBgColor(color_t color) = 0;
  virtual void setCurrentTool(Tool* tool) = 0;
  virtual void setTiledMode(TiledMode mode) = 0;

  // Grid settings

  virtual bool getSnapToGrid() = 0;
  virtual bool getGridVisible() = 0;
  virtual Rect getGridBounds() = 0;
  virtual color_t getGridColor() = 0;

  virtual void setSnapToGrid(bool state) = 0;
  virtual void setGridVisible(bool state) = 0;
  virtual void setGridBounds(Rect rect) = 0;
  virtual void setGridColor(color_t color) = 0;

  // Pixel grid

  virtual bool getPixelGridVisible() = 0;
  virtual color_t getPixelGridColor() = 0;

  virtual void setPixelGridVisible(bool state) = 0;
  virtual void setPixelGridColor(color_t color) = 0;

  // Onionskin settings

  virtual bool getUseOnionskin() = 0;
  virtual int getOnionskinPrevFrames() = 0;
  virtual int getOnionskinNextFrames() = 0;
  virtual int getOnionskinOpacityBase() = 0;
  virtual int getOnionskinOpacityStep() = 0;

  virtual void setUseOnionskin(bool state) = 0;
  virtual void setOnionskinPrevFrames(int frames) = 0;
  virtual void setOnionskinNextFrames(int frames) = 0;
  virtual void setOnionskinOpacityBase(int base) = 0;
  virtual void setOnionskinOpacityStep(int step) = 0;

  // Tools settings

  virtual IToolSettings* getToolSettings(Tool* tool) = 0;
  
};

// Tool's settings
class IToolSettings
{
public:
  virtual ~IToolSettings() { }

  virtual IPenSettings* getPen() = 0;

  virtual int getOpacity() = 0;
  virtual int getTolerance() = 0;
  virtual bool getFilled() = 0;
  virtual bool getPreviewFilled() = 0;
  virtual int getSprayWidth() = 0;
  virtual int getSpraySpeed() = 0;

  virtual void setOpacity(int opacity) = 0;
  virtual void setTolerance(int tolerance) = 0;
  virtual void setFilled(bool state) = 0;
  virtual void setSprayWidth(int width) = 0;
  virtual void setSpraySpeed(int speed) = 0;
};

// Settings for a tool's pen
class IPenSettings
{
public:
  virtual ~IPenSettings() { }

  virtual PenType getType() = 0;
  virtual int getSize() = 0;
  virtual int getAngle() = 0;

  virtual void setType(PenType type) = 0;
  virtual void setSize(int size) = 0;
  virtual void setAngle(int angle) = 0;
};

#endif

