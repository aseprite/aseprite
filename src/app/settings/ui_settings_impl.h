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

#ifndef APP_SETTINGS_UI_SETTINGS_H_INCLUDED
#define APP_SETTINGS_UI_SETTINGS_H_INCLUDED
#pragma once

#include <map>
#include <string>
#include <vector>

#include "app/settings/settings.h"
#include "app/settings/settings_observers.h"
#include "base/compiler_specific.h"
#include "base/observable.h"
#include "base/unique_ptr.h"

namespace app {

  class UISettingsImpl
      : public ISettings
      , public IColorSwatchesStore
      , base::Observable<GlobalSettingsObserver> {
  public:
    UISettingsImpl();
    ~UISettingsImpl();

    // ISettings implementation
    bool getZoomWithScrollWheel() OVERRIDE;
    bool getShowSpriteEditorScrollbars() OVERRIDE;
    bool getGrabAlpha() OVERRIDE;
    app::Color getFgColor() OVERRIDE;
    app::Color getBgColor() OVERRIDE;
    tools::Tool* getCurrentTool() OVERRIDE;
    app::ColorSwatches* getColorSwatches() OVERRIDE;

    void setZoomWithScrollWheel(bool state) OVERRIDE;
    void setShowSpriteEditorScrollbars(bool state) OVERRIDE;
    void setGrabAlpha(bool state) OVERRIDE;
    void setFgColor(const app::Color& color) OVERRIDE;
    void setBgColor(const app::Color& color) OVERRIDE;
    void setCurrentTool(tools::Tool* tool) OVERRIDE;
    void setColorSwatches(app::ColorSwatches* colorSwatches) OVERRIDE;

    IDocumentSettings* getDocumentSettings(const Document* document) OVERRIDE;
    IToolSettings* getToolSettings(tools::Tool* tool) OVERRIDE;
    IColorSwatchesStore* getColorSwatchesStore() OVERRIDE;

    ISelectionSettings* selection() OVERRIDE;

    // IColorSwatchesStore implementation

    void addColorSwatches(app::ColorSwatches* colorSwatches) OVERRIDE;
    void removeColorSwatches(app::ColorSwatches* colorSwatches) OVERRIDE;

    void addObserver(GlobalSettingsObserver* observer) OVERRIDE;
    void removeObserver(GlobalSettingsObserver* observer) OVERRIDE;

  private:
    tools::Tool* m_currentTool;
    base::UniquePtr<IDocumentSettings> m_globalDocumentSettings;
    std::map<std::string, IToolSettings*> m_toolSettings;
    app::ColorSwatches* m_colorSwatches;
    std::vector<app::ColorSwatches*> m_colorSwatchesStore;
    base::UniquePtr<ISelectionSettings> m_selectionSettings;
    bool m_zoomWithScrollWheel;
    bool m_showSpriteEditorScrollbars;
    bool m_grabAlpha;
  };

} // namespace app

#endif
