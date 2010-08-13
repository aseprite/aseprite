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

#include "config.h"

#include <allegro/color.h>
#include <string>

#include "app.h"
#include "core/cfg.h"
#include "settings/ui_settings_impl.h"
#include "ui_context.h"
#include "tools/tool.h"
#include "tools/toolbox.h"
#include "widgets/colbar.h"

//////////////////////////////////////////////////////////////////////
// UISettingsImpl

UISettingsImpl::UISettingsImpl()
  : m_gridBounds(0, 0, 16, 16)
{
  m_currentTool = NULL;
  m_tiledMode     = (TiledMode)get_config_int("Tools", "Tiled", (int)TILED_NONE);
  m_tiledMode     = (TiledMode)MID(0, (int)m_tiledMode, (int)TILED_BOTH);
  m_snapToGrid    = get_config_bool("Grid", "SnapTo", false);
  m_gridVisible   = get_config_bool("Grid", "Visible", false);
  m_gridColor     = get_config_color("Grid", "Color", color_rgb(0, 0, 255));
  m_gridBounds    = get_config_rect("Grid", "Bounds", m_gridBounds);
  m_pixelGridVisible = get_config_bool("PixelGrid", "Visible", false);
  m_pixelGridColor   = get_config_color("PixelGrid", "Color", color_rgb(200, 200, 200));

  m_use_onionskin = get_config_bool("Onionskin", "Enabled", false);
  m_prev_frames_onionskin = get_config_int("Onionskin", "PrevFrames", 1);
  m_next_frames_onionskin = get_config_int("Onionskin", "NextFrames", 0);
  m_onionskin_opacity_base = get_config_int("Onionskin", "OpacityBase", 128);
  m_onionskin_opacity_step = get_config_int("Onionskin", "OpacityStep", 32);
}

UISettingsImpl::~UISettingsImpl()
{
  set_config_int("Tools", "Tiled", m_tiledMode);
  set_config_bool("Grid", "SnapTo", m_snapToGrid);
  set_config_bool("Grid", "Visible", m_gridVisible);
  set_config_rect("Grid", "Bounds", m_gridBounds);
  set_config_color("Grid", "Color", m_gridColor);
  set_config_bool("PixelGrid", "Visible", m_pixelGridVisible);
  set_config_color("PixelGrid", "Color", m_pixelGridColor);

  set_config_bool("Onionskin", "Enabled", m_use_onionskin);
  set_config_int("Onionskin", "PrevFrames", m_prev_frames_onionskin);
  set_config_int("Onionskin", "NextFrames", m_next_frames_onionskin);
  set_config_int("Onionskin", "OpacityBase", m_onionskin_opacity_base);
  set_config_int("Onionskin", "OpacityStep", m_onionskin_opacity_step);

  // delete all tool settings
  std::map<std::string, IToolSettings*>::iterator it;
  for (it = m_toolSettings.begin(); it != m_toolSettings.end(); ++it)
    delete it->second;
}

//////////////////////////////////////////////////////////////////////
// General settings

color_t UISettingsImpl::getFgColor()
{
  return app_get_colorbar()->getFgColor();
}

color_t UISettingsImpl::getBgColor()
{
  return app_get_colorbar()->getBgColor();
}

Tool* UISettingsImpl::getCurrentTool()
{
  if (!m_currentTool)
    m_currentTool = App::instance()->get_toolbox()->getToolById("pencil");

  return m_currentTool;
}

TiledMode UISettingsImpl::getTiledMode()
{
  return m_tiledMode;
}

void UISettingsImpl::setFgColor(color_t color)
{
  app_get_colorbar()->setFgColor(color);
}

void UISettingsImpl::setBgColor(color_t color)
{
  app_get_colorbar()->setFgColor(color);
}

void UISettingsImpl::setCurrentTool(Tool* tool)
{
  if (m_currentTool != tool) {
    // Fire PenSizeBeforeChange signal (maybe the new selected tool has a different pen size)
    App::instance()->PenSizeBeforeChange();
    
    // Change the tool
    m_currentTool = tool;

    App::instance()->CurrentToolChange(); // Fire CurrentToolChange signal
    App::instance()->PenSizeAfterChange(); // Fire PenSizeAfterChange signal
  }
}

void UISettingsImpl::setTiledMode(TiledMode mode)
{
  m_tiledMode = mode;
}

//////////////////////////////////////////////////////////////////////
// Grid settings

bool UISettingsImpl::getSnapToGrid()
{
  return m_snapToGrid;
}

bool UISettingsImpl::getGridVisible()
{
  return m_gridVisible;
}

Rect UISettingsImpl::getGridBounds()
{
  return m_gridBounds;
}

color_t UISettingsImpl::getGridColor()
{
  return m_gridColor;
}

void UISettingsImpl::setSnapToGrid(bool state)
{
  m_snapToGrid = state;
}

void UISettingsImpl::setGridVisible(bool state)
{
  m_gridVisible = state;
}

void UISettingsImpl::setGridBounds(Rect rect)
{
  m_gridBounds = rect;
}

void UISettingsImpl::setGridColor(color_t color)
{
  m_gridColor = color;
}

//////////////////////////////////////////////////////////////////////
// Pixel grid

bool UISettingsImpl::getPixelGridVisible()
{
  return m_pixelGridVisible;
}

color_t UISettingsImpl::getPixelGridColor()
{
  return m_pixelGridColor;
}

void UISettingsImpl::setPixelGridVisible(bool state)
{
  m_pixelGridVisible = state;
}

void UISettingsImpl::setPixelGridColor(color_t color)
{
  m_pixelGridColor = color;
}

//////////////////////////////////////////////////////////////////////
// Onionskin settings

bool UISettingsImpl::getUseOnionskin()
{
  return m_use_onionskin;
}

int UISettingsImpl::getOnionskinPrevFrames()
{
  return m_prev_frames_onionskin;
}

int UISettingsImpl::getOnionskinNextFrames()
{
  return m_next_frames_onionskin;
}

int UISettingsImpl::getOnionskinOpacityBase()
{
  return m_onionskin_opacity_base;
}

int UISettingsImpl::getOnionskinOpacityStep()
{
  return m_onionskin_opacity_step;
}

void UISettingsImpl::setUseOnionskin(bool state)
{
  m_use_onionskin = state;
}

void UISettingsImpl::setOnionskinPrevFrames(int frames)
{
  m_prev_frames_onionskin = frames;
}

void UISettingsImpl::setOnionskinNextFrames(int frames)
{
  m_next_frames_onionskin = frames;
}

void UISettingsImpl::setOnionskinOpacityBase(int base)
{
  m_onionskin_opacity_base = base;
}

void UISettingsImpl::setOnionskinOpacityStep(int step)
{
  m_onionskin_opacity_step = step;
}

//////////////////////////////////////////////////////////////////////
// Tools & pen settings

class UIPenSettingsImpl : public IPenSettings
{
  PenType m_type;
  int m_size;
  int m_angle;
  bool m_fireSignals;

public:
  UIPenSettingsImpl()
  {
    m_type = PEN_TYPE_FIRST;
    m_size = 1;
    m_angle = 0;
    m_fireSignals = true;
  }

  ~UIPenSettingsImpl()
  {
  }

  PenType getType() { return m_type; }
  int getSize() { return m_size; }
  int getAngle() { return m_angle; }

  void setType(PenType type) 
  {
    m_type = MID(PEN_TYPE_FIRST, type, PEN_TYPE_LAST);
  }

  void setSize(int size)
  {
    // Trigger PenSizeBeforeChange signal
    if (m_fireSignals)
      App::instance()->PenSizeBeforeChange();

    // Change the size of the pencil
    m_size = MID(1, size, 32);

    // Trigger PenSizeAfterChange signal
    if (m_fireSignals)
      App::instance()->PenSizeAfterChange();
  }

  void setAngle(int angle)
  {
    m_angle = MID(0, angle, 360);
  }

  void enableSignals(bool state)
  {
    m_fireSignals = state;
  }

};

class UIToolSettingsImpl : public IToolSettings
{
  Tool* m_tool;
  UIPenSettingsImpl m_pen;
  int m_opacity;
  int m_tolerance;
  bool m_filled;
  bool m_previewFilled;
  int m_spray_width;
  int m_spray_speed;

public:

  UIToolSettingsImpl(Tool* tool)
    : m_tool(tool)
  {
    std::string cfg_section(getCfgSection());

    m_opacity = get_config_int(cfg_section.c_str(), "Opacity", 255);
    m_opacity = MID(0, m_opacity, 255);
    m_tolerance = get_config_int(cfg_section.c_str(), "Tolerance", 0);
    m_tolerance = MID(0, m_tolerance, 255);
    m_filled = false;
    m_previewFilled = get_config_bool(cfg_section.c_str(), "PreviewFilled", false);
    m_spray_width = 16;
    m_spray_speed = 32;

    m_pen.enableSignals(false);
    m_pen.setType((PenType)get_config_int(cfg_section.c_str(), "PenType", (int)PEN_TYPE_CIRCLE));
    m_pen.setSize(get_config_int(cfg_section.c_str(), "PenSize", m_tool->getDefaultPenSize()));
    m_pen.setAngle(get_config_int(cfg_section.c_str(), "PenAngle", 0));
    m_pen.enableSignals(true);

    if (m_tool->getPointShape(0)->isSpray() ||
	m_tool->getPointShape(1)->isSpray()) {
      m_spray_width = get_config_int(cfg_section.c_str(), "SprayWidth", m_spray_width);
      m_spray_speed = get_config_int(cfg_section.c_str(), "SpraySpeed", m_spray_speed);
    }
  }

  ~UIToolSettingsImpl()
  {
    std::string cfg_section(getCfgSection());

    set_config_int(cfg_section.c_str(), "Opacity", m_opacity);
    set_config_int(cfg_section.c_str(), "Tolerance", m_tolerance);
    set_config_int(cfg_section.c_str(), "PenType", m_pen.getType());
    set_config_int(cfg_section.c_str(), "PenSize", m_pen.getSize());
    set_config_int(cfg_section.c_str(), "PenAngle", m_pen.getAngle());

    if (m_tool->getPointShape(0)->isSpray() ||
	m_tool->getPointShape(1)->isSpray()) {
      set_config_int(cfg_section.c_str(), "SprayWidth", m_spray_width);
      set_config_int(cfg_section.c_str(), "SpraySpeed", m_spray_speed);
    }

    set_config_bool(cfg_section.c_str(), "PreviewFilled", m_previewFilled);
  }

  IPenSettings* getPen() { return &m_pen; }

  int getOpacity() { return m_opacity; }
  int getTolerance() { return m_tolerance; }
  bool getFilled() { return m_filled; }
  bool getPreviewFilled() { return m_previewFilled; }
  int getSprayWidth() { return m_spray_width; }
  int getSpraySpeed() { return m_spray_speed; }

  void setOpacity(int opacity) { m_opacity = opacity; }
  void setTolerance(int tolerance) { m_tolerance = tolerance; }
  void setFilled(bool state) { m_filled = state; }
  void setPreviewFilled(bool state) { m_previewFilled = state; }
  void setSprayWidth(int width) { m_spray_width = width; }
  void setSpraySpeed(int speed) { m_spray_speed = speed; }

private:
  std::string getCfgSection() const {
    return std::string("Tool:") + m_tool->getId();
  }
};

IToolSettings* UISettingsImpl::getToolSettings(Tool* tool)
{
  ASSERT(tool != NULL);

  std::map<std::string, IToolSettings*>::iterator
    it = m_toolSettings.find(tool->getId());

  if (it != m_toolSettings.end()) {
    return it->second;
  }
  else {
    IToolSettings* tool_settings = new UIToolSettingsImpl(tool);
    m_toolSettings[tool->getId()] = tool_settings;
    return tool_settings;
  }
}
