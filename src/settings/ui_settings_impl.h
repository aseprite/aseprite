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

#ifndef SETTINGS_UI_SETTINGS_H_INCLUDED
#define SETTINGS_UI_SETTINGS_H_INCLUDED

#include <map>
#include <string>
#include "settings/settings.h"

class UISettingsImpl : public ISettings
{
public:
  UISettingsImpl();
  ~UISettingsImpl();

  // General settings

  color_t getFgColor();
  color_t getBgColor();
  Tool* getCurrentTool();
  TiledMode getTiledMode();

  void setFgColor(color_t color);
  void setBgColor(color_t color);
  void setCurrentTool(Tool* tool);
  void setTiledMode(TiledMode mode);

  // Grid settings

  bool getSnapToGrid();
  bool getGridVisible();
  Rect getGridBounds();
  color_t getGridColor();

  void setSnapToGrid(bool state);
  void setGridVisible(bool state);
  void setGridBounds(Rect rect);
  void setGridColor(color_t color);

  // Pixel grid

  bool getPixelGridVisible();
  color_t getPixelGridColor();

  void setPixelGridVisible(bool state);
  void setPixelGridColor(color_t color);

  // Onionskin settings

  bool getUseOnionskin();
  int getOnionskinPrevFrames();
  int getOnionskinNextFrames();
  int getOnionskinOpacityBase();
  int getOnionskinOpacityStep();

  void setUseOnionskin(bool state);
  void setOnionskinPrevFrames(int frames);
  void setOnionskinNextFrames(int frames);
  void setOnionskinOpacityBase(int base);
  void setOnionskinOpacityStep(int step);

  // Tools settings

  IToolSettings* getToolSettings(Tool* tool);

private:
  TiledMode m_tiledMode;
  Tool* m_currentTool;
  bool m_use_onionskin;
  int m_prev_frames_onionskin;
  int m_next_frames_onionskin;
  int m_onionskin_opacity_base;
  int m_onionskin_opacity_step;
  bool m_snapToGrid;
  bool m_gridVisible;
  Rect m_gridBounds;
  color_t m_gridColor;
  bool m_pixelGridVisible;
  color_t m_pixelGridColor;
  std::map<std::string, IToolSettings*> m_toolSettings;
};

#endif
