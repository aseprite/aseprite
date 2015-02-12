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
#include "doc/documents_observer.h"
#include "doc/object_id.h"

namespace app {

  class UISettingsImpl
      : public ISettings
      , public IExperimentalSettings
      , public IColorSwatchesStore
      , public doc::DocumentsObserver
      , base::Observable<GlobalSettingsObserver> {
  public:
    UISettingsImpl();
    ~UISettingsImpl();

    // doc::DocumentsObserver
    void onRemoveDocument(doc::Document* doc) override;

    // ISettings implementation
    bool getZoomWithScrollWheel() override;
    bool getCenterOnZoom() override;
    bool getShowSpriteEditorScrollbars() override;
    RightClickMode getRightClickMode() override;
    bool getGrabAlpha() override;
    bool getAutoSelectLayer() override;
    app::Color getFgColor() override;
    app::Color getBgColor() override;
    tools::Tool* getCurrentTool() override;
    app::ColorSwatches* getColorSwatches() override;

    void setZoomWithScrollWheel(bool state) override;
    void setCenterOnZoom(bool state) override;
    void setShowSpriteEditorScrollbars(bool state) override;
    void setRightClickMode(RightClickMode mode) override;
    void setGrabAlpha(bool state) override;
    void setAutoSelectLayer(bool state) override;
    void setFgColor(const app::Color& color) override;
    void setBgColor(const app::Color& color) override;
    void setCurrentTool(tools::Tool* tool) override;
    void setColorSwatches(app::ColorSwatches* colorSwatches) override;

    IDocumentSettings* getDocumentSettings(const doc::Document* document) override;
    IToolSettings* getToolSettings(tools::Tool* tool) override;
    IColorSwatchesStore* getColorSwatchesStore() override;

    ISelectionSettings* selection() override;
    IExperimentalSettings* experimental() override;

    // IExperimentalSettings implementation

    bool useNativeCursor() const override;
    void setUseNativeCursor(bool state) override;

    bool flashLayer() const override;
    void setFlashLayer(bool state) override;

    // IColorSwatchesStore implementation

    void addColorSwatches(app::ColorSwatches* colorSwatches) override;
    void removeColorSwatches(app::ColorSwatches* colorSwatches) override;

    void addObserver(GlobalSettingsObserver* observer) override;
    void removeObserver(GlobalSettingsObserver* observer) override;

  private:
    tools::Tool* m_currentTool;
    std::map<std::string, IToolSettings*> m_toolSettings;
    app::ColorSwatches* m_colorSwatches;
    std::vector<app::ColorSwatches*> m_colorSwatchesStore;
    base::UniquePtr<ISelectionSettings> m_selectionSettings;
    bool m_zoomWithScrollWheel;
    bool m_centerOnZoom;
    bool m_showSpriteEditorScrollbars;
    bool m_grabAlpha;
    bool m_autoSelectLayer;
    RightClickMode m_rightClickMode;
    std::map<doc::ObjectId, IDocumentSettings*> m_docSettings;
  };

} // namespace app

#endif
