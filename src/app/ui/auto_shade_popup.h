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
#include "ui/popup_window.h"
#include "ui/slider.h"
#include "ui/view.h"

#include "obs/signal.h"

namespace app {

class AutoShadePopup : public ui::PopupWindow {
public:
    AutoShadePopup();

    // Get/set configuration
    tools::ShadeConfig getConfig() const;
    void setConfig(const tools::ShadeConfig& config);

    // Signal emitted when settings change
    obs::signal<void(const tools::ShadeConfig&)> ConfigChange;

private:
    void onLightAngleChange();
    void onFlipHorizontal();
    void onFlipVertical();
    void onAmbientChange();
    void onLightElevationChange();
    void onRoundnessChange();
    void onHighlightFocusChange();
    void onShadingModeChange();
    void onNormalMethodChange();
    void onShapeTypeChange();
    void onFillModeChange();
    void onColorSourceChange();
    void onShadingStyleChange();
    void onColorChange();
    void onOutlineColorChange();
    void onShowOutlineChange();
    void onSelectiveOutlineChange();
    void onAntiAliasingChange();
    void onDitheringChange();

    void notifyChange();

    // UI widgets - Light settings
    ui::Slider* m_lightAngleSlider;
    ui::Label* m_lightAngleLabel;
    ui::Button* m_flipHButton;
    ui::Button* m_flipVButton;
    ui::Slider* m_ambientSlider;
    ui::Label* m_ambientLabel;
    ui::Slider* m_lightElevationSlider;
    ui::Label* m_lightElevationLabel;
    ui::Slider* m_roundnessSlider;
    ui::Label* m_roundnessLabel;

    // UI widgets - Shading options
    ui::ComboBox* m_shadingModeCombo;
    ui::ComboBox* m_normalMethodCombo;
    ui::ComboBox* m_shapeTypeCombo;
    ui::ComboBox* m_fillModeCombo;
    ui::ComboBox* m_colorSourceCombo;
    ui::ComboBox* m_shadingStyleCombo;

    // UI widgets - Colors
    ColorButton* m_shadowColorBtn;
    ColorButton* m_baseColorBtn;
    ColorButton* m_highlightColorBtn;

    // UI widgets - Outline
    ui::CheckBox* m_showOutlineCheck;
    ui::CheckBox* m_selectiveOutlineCheck;
    ColorButton* m_lightOutlineColorBtn;
    ColorButton* m_shadowOutlineColorBtn;

    // UI widgets - Highlight focus
    ui::Slider* m_highlightFocusSlider;
    ui::Label* m_highlightFocusLabel;

    // UI widgets - Advanced options
    ui::CheckBox* m_antiAliasingCheck;
    ui::CheckBox* m_ditheringCheck;

    // Scrollable view container
    ui::View* m_view;

    // Current config (to avoid unnecessary updates)
    tools::ShadeConfig m_config;

    // Guard flag to prevent feedback loops during setConfig
    bool m_updatingFromCode = false;
};

} // namespace app

#endif // APP_UI_AUTO_SHADE_POPUP_H_INCLUDED
