// Aseprite
// Copyright (C) 2026  Custom Build
//
// Standalone Palette Generator Tool
// Generates shading palettes from a base color using color theory

#ifndef APP_UI_PALETTE_GENERATOR_POPUP_H_INCLUDED
#define APP_UI_PALETTE_GENERATOR_POPUP_H_INCLUDED
#pragma once

#include "app/tools/auto_shade/palette_generator.h"
#include "app/tools/auto_shade/shade_types.h"
#include "app/ui/color_button.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/combobox.h"
#include "ui/label.h"
#include "ui/slider.h"
#include "ui/window.h"

#include "obs/signal.h"

namespace app {

class PaletteGeneratorPopup : public ui::Window {
public:
    PaletteGeneratorPopup();

    // Signal emitted when palette is generated and should be applied
    obs::signal<void(const tools::GeneratedPalette&)> PaletteGenerated;

private:
    void onBaseColorChange();
    void onMaterialChange();
    void onStyleChange();
    void onLightAngleChange();
    void onColorCountChange();
    void onHueShiftChange();
    void onSaturationRangeChange();
    void onValueRangeChange();
    void onHarmonyChange();
    void onTemperatureChange();
    void onGenerateClick();
    void onAddToPaletteClick();
    void onReplacePaletteClick();
    void onSetAsFgBgClick();
    void onCopyHexClick();
    void onResetClick();
    void onPickColorClick();

    void updatePreview();
    void rebuildPreviewButtons();

    // UI widgets - Base settings
    ColorButton* m_baseColorBtn;
    ui::ComboBox* m_materialCombo;
    ui::ComboBox* m_styleCombo;
    ui::Slider* m_lightAngleSlider;
    ui::Label* m_lightAngleLabel;

    // UI widgets - Advanced controls
    ui::ComboBox* m_colorCountCombo;
    ui::Slider* m_hueShiftSlider;
    ui::Label* m_hueShiftLabel;
    ui::Slider* m_saturationRangeSlider;
    ui::Label* m_saturationRangeLabel;
    ui::Slider* m_valueRangeSlider;
    ui::Label* m_valueRangeLabel;
    ui::ComboBox* m_harmonyCombo;
    ui::ComboBox* m_temperatureCombo;

    // Preview area
    ui::HBox* m_previewBox;
    std::vector<ColorButton*> m_previewButtons;

    // Action buttons
    ui::Button* m_generateBtn;
    ui::Button* m_addToPaletteBtn;
    ui::Button* m_replacePaletteBtn;
    ui::Button* m_setFgBgBtn;
    ui::Button* m_copyHexBtn;
    ui::Button* m_resetBtn;
    ui::Button* m_pickColorBtn;

    // Current generated palette
    tools::GeneratedPalette m_currentPalette;

    // Settings
    tools::PaletteMaterial m_material;
    tools::PaletteStyle m_style;
    tools::PaletteSettings m_settings;
    double m_lightAngle;
};

} // namespace app

#endif // APP_UI_PALETTE_GENERATOR_POPUP_H_INCLUDED
