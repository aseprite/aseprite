// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_SETTINGS_UI_SETTINGS_H_INCLUDED
#define APP_SETTINGS_UI_SETTINGS_H_INCLUDED
#pragma once

#include <map>
#include <string>
#include <vector>

#include "app/settings/settings.h"
#include "app/settings/settings_observers.h"
#include "base/observable.h"
#include "base/unique_ptr.h"
#include "doc/object_id.h"

namespace app {

  class UISettingsImpl
      : public ISettings
      , public IColorSwatchesStore {
  public:
    UISettingsImpl();
    ~UISettingsImpl();

    // ISettings implementation
    bool getAutoSelectLayer() override;
    app::Color getFgColor() override;
    app::Color getBgColor() override;
    tools::Tool* getCurrentTool() override;
    app::ColorSwatches* getColorSwatches() override;

    void setAutoSelectLayer(bool state) override;
    void setFgColor(const app::Color& color) override;
    void setBgColor(const app::Color& color) override;
    void setCurrentTool(tools::Tool* tool) override;
    void setColorSwatches(app::ColorSwatches* colorSwatches) override;

    IToolSettings* getToolSettings(tools::Tool* tool) override;
    IColorSwatchesStore* getColorSwatchesStore() override;

    ISelectionSettings* selection() override;

    // IColorSwatchesStore implementation

    void addColorSwatches(app::ColorSwatches* colorSwatches) override;
    void removeColorSwatches(app::ColorSwatches* colorSwatches) override;

  private:
    tools::Tool* m_currentTool;
    std::map<std::string, IToolSettings*> m_toolSettings;
    app::ColorSwatches* m_colorSwatches;
    std::vector<app::ColorSwatches*> m_colorSwatchesStore;
    base::UniquePtr<ISelectionSettings> m_selectionSettings;
    bool m_grabAlpha;
    bool m_autoSelectLayer;
  };

} // namespace app

#endif
