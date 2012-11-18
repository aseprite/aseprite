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

#ifndef SETTINGS_DOCUMENT_SETTINGS_H_INCLUDED
#define SETTINGS_DOCUMENT_SETTINGS_H_INCLUDED

#include "app/color.h"
#include "filters/tiled_mode.h"
#include "gfx/point.h"
#include "gfx/rect.h"

enum SnapBehavior {
  NormalSnap = 0,
  SnapInRightBottom = 1
};

class IDocumentSettings
{
public:
  virtual ~IDocumentSettings() { }

  // Tiled mode

  virtual TiledMode getTiledMode() = 0;
  virtual void setTiledMode(TiledMode mode) = 0;

  // Grid settings

  virtual bool getSnapToGrid() = 0;
  virtual bool getGridVisible() = 0;
  virtual gfx::Rect getGridBounds() = 0;
  virtual Color getGridColor() = 0;

  virtual void setSnapToGrid(bool state) = 0;
  virtual void setGridVisible(bool state) = 0;
  virtual void setGridBounds(const gfx::Rect& rect) = 0;
  virtual void setGridColor(const Color& color) = 0;

  virtual void snapToGrid(gfx::Point& point, SnapBehavior snapBehavior) const = 0;

  // Pixel grid

  virtual bool getPixelGridVisible() = 0;
  virtual Color getPixelGridColor() = 0;

  virtual void setPixelGridVisible(bool state) = 0;
  virtual void setPixelGridColor(const Color& color) = 0;

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
};

#endif
