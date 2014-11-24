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

#ifndef APP_SETTINGS_DOCUMENT_SETTINGS_H_INCLUDED
#define APP_SETTINGS_DOCUMENT_SETTINGS_H_INCLUDED
#pragma once

#include "app/color.h"
#include "filters/tiled_mode.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "doc/frame_number.h"

namespace app {
  class DocumentSettingsObserver;

  class IDocumentSettings {
  public:
    virtual ~IDocumentSettings() { }

    // Tiled mode

    virtual filters::TiledMode getTiledMode() = 0;
    virtual void setTiledMode(filters::TiledMode mode) = 0;

    // Grid settings

    virtual bool getSnapToGrid() = 0;
    virtual bool getGridVisible() = 0;
    virtual gfx::Rect getGridBounds() = 0;
    virtual app::Color getGridColor() = 0;
    virtual int getGridOpacity() = 0;
    virtual bool getGridAutoOpacity() = 0;

    virtual void setSnapToGrid(bool state) = 0;
    virtual void setGridVisible(bool state) = 0;
    virtual void setGridBounds(const gfx::Rect& rect) = 0;
    virtual void setGridColor(const app::Color& color) = 0;
    virtual void setGridOpacity(int opacity) = 0;
    virtual void setGridAutoOpacity(bool state) = 0;

    virtual void snapToGrid(gfx::Point& point) const = 0;

    // Pixel grid

    virtual bool getPixelGridVisible() = 0;
    virtual app::Color getPixelGridColor() = 0;
    virtual int getPixelGridOpacity() = 0;
    virtual bool getPixelGridAutoOpacity() = 0;

    virtual void setPixelGridVisible(bool state) = 0;
    virtual void setPixelGridColor(const app::Color& color) = 0;
    virtual void setPixelGridOpacity(int opacity) = 0;
    virtual void setPixelGridAutoOpacity(bool state) = 0;

    // Onionskin settings

    enum OnionskinType {
      Onionskin_Merge,
      Onionskin_RedBlueTint,
      Onionskin_Last = Onionskin_RedBlueTint
    };

    virtual bool getUseOnionskin() = 0;
    virtual int getOnionskinPrevFrames() = 0;
    virtual int getOnionskinNextFrames() = 0;
    virtual int getOnionskinOpacityBase() = 0;
    virtual int getOnionskinOpacityStep() = 0;
    virtual OnionskinType getOnionskinType() = 0;

    virtual void setUseOnionskin(bool state) = 0;
    virtual void setOnionskinPrevFrames(int frames) = 0;
    virtual void setOnionskinNextFrames(int frames) = 0;
    virtual void setOnionskinOpacityBase(int base) = 0;
    virtual void setOnionskinOpacityStep(int step) = 0;
    virtual void setOnionskinType(OnionskinType type) = 0;
    virtual void setDefaultOnionskinSettings() = 0;

    // Animation

    enum AniDir {
      AniDir_Normal,
      AniDir_Reverse,
      AniDir_PingPong
    };

    virtual bool getLoopAnimation() = 0;
    virtual void getLoopRange(doc::FrameNumber* begin, doc::FrameNumber* end) = 0;
    virtual AniDir getAnimationDirection() = 0;

    virtual void setLoopAnimation(bool state) = 0;
    virtual void setLoopRange(doc::FrameNumber begin, doc::FrameNumber end) = 0;
    virtual void setAnimationDirection(AniDir dir) = 0;

    // Observable

    virtual void addObserver(DocumentSettingsObserver* observer) = 0;
    virtual void removeObserver(DocumentSettingsObserver* observer) = 0;
  };

} // namespace app

#endif
