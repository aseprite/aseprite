/* ASEPRITE
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

#ifndef SETTINGS_UI_SETTINGS_H_INCLUDED
#define SETTINGS_UI_SETTINGS_H_INCLUDED

#include <map>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "settings/settings.h"

class UISettingsImpl : public ISettings,
                       public IColorSwatchesStore
{
public:
  UISettingsImpl();
  ~UISettingsImpl();

  // ISettings implementation

  app::Color getFgColor() OVERRIDE;
  app::Color getBgColor() OVERRIDE;
  tools::Tool* getCurrentTool() OVERRIDE;
  app::ColorSwatches* getColorSwatches() OVERRIDE;
  void setFgColor(const app::Color& color) OVERRIDE;
  void setBgColor(const app::Color& color) OVERRIDE;
  void setCurrentTool(tools::Tool* tool) OVERRIDE;
  void setColorSwatches(app::ColorSwatches* colorSwatches) OVERRIDE;
  IDocumentSettings* getDocumentSettings(const Document* document) OVERRIDE;
  IToolSettings* getToolSettings(tools::Tool* tool) OVERRIDE;
  IColorSwatchesStore* getColorSwatchesStore() OVERRIDE;

  // IColorSwatchesStore implementation

  void addColorSwatches(app::ColorSwatches* colorSwatches) OVERRIDE;
  void removeColorSwatches(app::ColorSwatches* colorSwatches) OVERRIDE;

private:
  tools::Tool* m_currentTool;
  IDocumentSettings* m_globalDocumentSettings;
  std::map<std::string, IToolSettings*> m_toolSettings;
  app::ColorSwatches* m_colorSwatches;
  std::vector<app::ColorSwatches*> m_colorSwatchesStore;
};

#endif
