// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Options Popup Window

#ifndef APP_UI_AUTO_SHADE_POPUP_H_INCLUDED
#define APP_UI_AUTO_SHADE_POPUP_H_INCLUDED
#pragma once

#include "app/tools/auto_shade/shade_types.h"
#include "app/ui/color_button.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/label.h"
#include "ui/window.h"
#include "ui/slider.h"

#include "obs/signal.h"

namespace app {

class AutoShadePopup : public ui::Window {
public:
    AutoShadePopup();

    // Get/set configuration
    tools::ShadeConfig getConfig() const;
    void setConfig(const tools::ShadeConfig& config);

    // Signal emitted when settings change
    obs::signal<void(const tools::ShadeConfig&)> ConfigChange;

private:
    void onLightAngleChange();
    void onAmbientChange();
    void onLightElevationChange();
    void onRoundnessChange();
    void onShadingModeChange();
    void onNormalMethodChange();
    void onFillModeChange();
    void onColorChange();
    void onShowOutlineChange();

    void notifyChange();

    // UI widgets
    ui::Slider* m_lightAngleSlider;
    ui::Label* m_lightAngleLabel;
    ui::Slider* m_ambientSlider;
    ui::Label* m_ambientLabel;
    ui::Slider* m_lightElevationSlider;
    ui::Label* m_lightElevationLabel;
    ui::Slider* m_roundnessSlider;
    ui::Label* m_roundnessLabel;
    ui::ComboBox* m_shadingModeCombo;
    ui::ComboBox* m_normalMethodCombo;
    ui::ComboBox* m_fillModeCombo;
    ColorButton* m_shadowColorBtn;
    ColorButton* m_baseColorBtn;
    ColorButton* m_highlightColorBtn;
    ui::CheckBox* m_showOutlineCheck;

    // Current config (to avoid unnecessary updates)
    tools::ShadeConfig m_config;
};

} // namespace app

#endif // APP_UI_AUTO_SHADE_POPUP_H_INCLUDED
