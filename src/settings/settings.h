/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "app/color.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "raster/pen_type.h"

class Document;
class IDocumentSettings;
class IToolSettings;
class IPenSettings;

namespace tools { class Tool; }

class ISettings
{
public:
  virtual ~ISettings() { }

  // General settings

  virtual app::Color getFgColor() = 0;
  virtual app::Color getBgColor() = 0;
  virtual tools::Tool* getCurrentTool() = 0;

  virtual void setFgColor(const app::Color& color) = 0;
  virtual void setBgColor(const app::Color& color) = 0;
  virtual void setCurrentTool(tools::Tool* tool) = 0;

  // Returns the specific settings for the given document. If the
  // document is null, it should return an interface for
  // global/default settings.
  virtual IDocumentSettings* getDocumentSettings(const Document* document) = 0;

  // Specific configuration for the given tool.
  virtual IToolSettings* getToolSettings(tools::Tool* tool) = 0;

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
