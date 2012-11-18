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

#ifndef SETTINGS_UI_SETTINGS_H_INCLUDED
#define SETTINGS_UI_SETTINGS_H_INCLUDED

#include <map>
#include <string>
#include "base/compiler_specific.h"
#include "settings/settings.h"

class UISettingsImpl : public ISettings
{
public:
  UISettingsImpl();
  ~UISettingsImpl();

  // General settings

  Color getFgColor() OVERRIDE;
  Color getBgColor() OVERRIDE;
  tools::Tool* getCurrentTool() OVERRIDE;

  void setFgColor(const Color& color) OVERRIDE;
  void setBgColor(const Color& color) OVERRIDE;
  void setCurrentTool(tools::Tool* tool) OVERRIDE;

  // Document settings

  IDocumentSettings* getDocumentSettings(const Document* document) OVERRIDE;

  // Tools settings

  IToolSettings* getToolSettings(tools::Tool* tool) OVERRIDE;

private:
  tools::Tool* m_currentTool;
  IDocumentSettings* m_globalDocumentSettings;
  std::map<std::string, IToolSettings*> m_toolSettings;
};

#endif
