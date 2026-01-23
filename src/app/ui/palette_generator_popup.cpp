// Aseprite
// Copyright (C) 2026  Custom Build
//
// Standalone Palette Generator Tool

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/palette_generator_popup.h"

#include "app/app.h"
#include "app/cmd/set_palette.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/context.h"
#include "app/context_access.h"
#include "app/doc.h"
#include "app/modules/palettes.h"
#include "app/pref/preferences.h"
#include "app/tx.h"
#include "app/ui/color_bar.h"
#include "app/ui/skin/skin_theme.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "ui/box.h"
#include "ui/grid.h"
#include "ui/separator.h"
#include "ui/view.h"

#include "clip/clip.h"
#include "fmt/format.h"

namespace app {

using namespace ui;
using namespace skin;

PaletteGeneratorPopup::PaletteGeneratorPopup()
    : Window(Window::WithTitleBar, "Palette Generator")
    , m_material(tools::PaletteMaterial::Matte)
    , m_style(tools::PaletteStyle::Natural)
    , m_lightAngle(135.0)
    , m_previewBox(nullptr)
{
    auto theme = SkinTheme::get(this);

    // Main vertical box
    auto mainBox = new VBox();
    mainBox->setExpansive(true);

    // ---- Base Color Section ----
    auto baseSection = new VBox();
    baseSection->addChild(new Label("Base Color"));
    baseSection->addChild(new Separator("", HORIZONTAL));

    ColorButtonOptions colorOpts;
    colorOpts.canPinSelector = false;
    colorOpts.showSimpleColors = false;
    colorOpts.showIndexTab = true;

    auto baseColorBox = new HBox();
    baseColorBox->addChild(new Label("Color:"));

    // Get current foreground color as default
    app::Color fgColor = Preferences::instance().colorBar.fgColor();
    m_baseColorBtn = new ColorButton(fgColor, doc::IMAGE_RGB, colorOpts);
    m_baseColorBtn->setExpansive(true);
    baseColorBox->addChild(m_baseColorBtn);

    // Eyedropper button to pick color from canvas
    m_pickColorBtn = new Button("Pick");
    m_pickColorBtn->setMinSize(gfx::Size(40, 0));
    baseColorBox->addChild(m_pickColorBtn);

    baseSection->addChild(baseColorBox);

    mainBox->addChild(baseSection);

    // ---- Settings Section ----
    auto settingsSection = new VBox();
    settingsSection->addChild(new Label("Settings"));
    settingsSection->addChild(new Separator("", HORIZONTAL));

    auto settingsGrid = new ui::Grid(2, false);

    // Material
    settingsGrid->addChildInCell(new Label("Material:"), 1, 1, 0);
    m_materialCombo = new ComboBox();
    m_materialCombo->addItem("Matte");
    m_materialCombo->addItem("Glossy");
    m_materialCombo->addItem("Metallic");
    m_materialCombo->addItem("Skin");
    m_materialCombo->setSelectedItemIndex(0);
    m_materialCombo->setExpansive(true);
    settingsGrid->addChildInCell(m_materialCombo, 1, 1, 0);

    // Style
    settingsGrid->addChildInCell(new Label("Style:"), 1, 1, 0);
    m_styleCombo = new ComboBox();
    m_styleCombo->addItem("Natural");
    m_styleCombo->addItem("Vibrant");
    m_styleCombo->addItem("Muted");
    m_styleCombo->addItem("Dramatic");
    m_styleCombo->setSelectedItemIndex(0);
    m_styleCombo->setExpansive(true);
    settingsGrid->addChildInCell(m_styleCombo, 1, 1, 0);

    // Color Count
    settingsGrid->addChildInCell(new Label("Colors:"), 1, 1, 0);
    m_colorCountCombo = new ComboBox();
    m_colorCountCombo->addItem("3 Colors");
    m_colorCountCombo->addItem("5 Colors");
    m_colorCountCombo->addItem("7 Colors");
    m_colorCountCombo->addItem("9 Colors");
    m_colorCountCombo->setSelectedItemIndex(1);  // Default 5 colors
    m_colorCountCombo->setExpansive(true);
    settingsGrid->addChildInCell(m_colorCountCombo, 1, 1, 0);

    // Harmony
    settingsGrid->addChildInCell(new Label("Harmony:"), 1, 1, 0);
    m_harmonyCombo = new ComboBox();
    m_harmonyCombo->addItem("Monochromatic");
    m_harmonyCombo->addItem("Analogous");
    m_harmonyCombo->addItem("Complementary");
    m_harmonyCombo->addItem("Split-Compl.");
    m_harmonyCombo->addItem("Triadic");
    m_harmonyCombo->addItem("Tetradic");
    m_harmonyCombo->setSelectedItemIndex(0);
    m_harmonyCombo->setExpansive(true);
    settingsGrid->addChildInCell(m_harmonyCombo, 1, 1, 0);

    // Temperature
    settingsGrid->addChildInCell(new Label("Temperature:"), 1, 1, 0);
    m_temperatureCombo = new ComboBox();
    m_temperatureCombo->addItem("Warm Highlights");
    m_temperatureCombo->addItem("Cool Highlights");
    m_temperatureCombo->addItem("Neutral");
    m_temperatureCombo->setSelectedItemIndex(0);
    m_temperatureCombo->setExpansive(true);
    settingsGrid->addChildInCell(m_temperatureCombo, 1, 1, 0);

    settingsSection->addChild(settingsGrid);

    // Light angle
    auto lightAngleBox = new HBox();
    lightAngleBox->addChild(new Label("Light Angle:"));
    m_lightAngleSlider = new Slider(0, 360, static_cast<int>(m_lightAngle));
    m_lightAngleSlider->setExpansive(true);
    m_lightAngleLabel = new Label(fmt::format("{:.0f}°", m_lightAngle));
    m_lightAngleLabel->setMinSize(gfx::Size(35, 0));
    lightAngleBox->addChild(m_lightAngleSlider);
    lightAngleBox->addChild(m_lightAngleLabel);
    settingsSection->addChild(lightAngleBox);

    // Hue Shift
    auto hueShiftBox = new HBox();
    hueShiftBox->addChild(new Label("Hue Shift:"));
    m_hueShiftSlider = new Slider(0, 60, static_cast<int>(m_settings.hueShift));
    m_hueShiftSlider->setExpansive(true);
    m_hueShiftLabel = new Label(fmt::format("{:.0f}°", m_settings.hueShift));
    m_hueShiftLabel->setMinSize(gfx::Size(35, 0));
    hueShiftBox->addChild(m_hueShiftSlider);
    hueShiftBox->addChild(m_hueShiftLabel);
    settingsSection->addChild(hueShiftBox);

    // Saturation Range
    auto satRangeBox = new HBox();
    satRangeBox->addChild(new Label("Sat. Range:"));
    m_saturationRangeSlider = new Slider(0, 100, static_cast<int>(m_settings.saturationRange * 100));
    m_saturationRangeSlider->setExpansive(true);
    m_saturationRangeLabel = new Label(fmt::format("{:.0f}%", m_settings.saturationRange * 100));
    m_saturationRangeLabel->setMinSize(gfx::Size(35, 0));
    satRangeBox->addChild(m_saturationRangeSlider);
    satRangeBox->addChild(m_saturationRangeLabel);
    settingsSection->addChild(satRangeBox);

    // Value Range
    auto valRangeBox = new HBox();
    valRangeBox->addChild(new Label("Value Range:"));
    m_valueRangeSlider = new Slider(20, 100, static_cast<int>(m_settings.valueRange * 100));
    m_valueRangeSlider->setExpansive(true);
    m_valueRangeLabel = new Label(fmt::format("{:.0f}%", m_settings.valueRange * 100));
    m_valueRangeLabel->setMinSize(gfx::Size(35, 0));
    valRangeBox->addChild(m_valueRangeSlider);
    valRangeBox->addChild(m_valueRangeLabel);
    settingsSection->addChild(valRangeBox);

    mainBox->addChild(settingsSection);

    // ---- Preview Section ----
    auto previewSection = new VBox();
    previewSection->addChild(new Label("Generated Palette"));
    previewSection->addChild(new Separator("", HORIZONTAL));

    m_previewBox = new HBox();
    previewSection->addChild(m_previewBox);

    mainBox->addChild(previewSection);

    // ---- Action Buttons ----
    auto actionsSection = new VBox();
    actionsSection->addChild(new Separator("", HORIZONTAL));

    auto actionRow1 = new HBox();
    m_addToPaletteBtn = new Button("Add to Palette");
    m_addToPaletteBtn->setExpansive(true);
    m_replacePaletteBtn = new Button("Replace Palette");
    m_replacePaletteBtn->setExpansive(true);
    actionRow1->addChild(m_addToPaletteBtn);
    actionRow1->addChild(m_replacePaletteBtn);
    actionsSection->addChild(actionRow1);

    auto actionRow2 = new HBox();
    m_setFgBgBtn = new Button("Set FG/BG");
    m_setFgBgBtn->setExpansive(true);
    m_copyHexBtn = new Button("Copy Hex");
    m_copyHexBtn->setExpansive(true);
    actionRow2->addChild(m_setFgBgBtn);
    actionRow2->addChild(m_copyHexBtn);
    actionsSection->addChild(actionRow2);

    auto actionRow3 = new HBox();
    m_resetBtn = new Button("Reset");
    m_resetBtn->setExpansive(true);
    actionRow3->addChild(m_resetBtn);
    actionsSection->addChild(actionRow3);

    mainBox->addChild(actionsSection);

    // Wrap in scrollable view
    auto view = new View();
    view->attachToView(mainBox);
    view->setExpansive(true);
    addChild(view);

    // Connect signals
    m_baseColorBtn->Change.connect([this]() { onBaseColorChange(); });
    m_materialCombo->Change.connect([this]() { onMaterialChange(); });
    m_styleCombo->Change.connect([this]() { onStyleChange(); });
    m_colorCountCombo->Change.connect([this]() { onColorCountChange(); });
    m_harmonyCombo->Change.connect([this]() { onHarmonyChange(); });
    m_temperatureCombo->Change.connect([this]() { onTemperatureChange(); });
    m_lightAngleSlider->Change.connect([this]() { onLightAngleChange(); });
    m_hueShiftSlider->Change.connect([this]() { onHueShiftChange(); });
    m_saturationRangeSlider->Change.connect([this]() { onSaturationRangeChange(); });
    m_valueRangeSlider->Change.connect([this]() { onValueRangeChange(); });
    m_addToPaletteBtn->Click.connect([this]() { onAddToPaletteClick(); });
    m_replacePaletteBtn->Click.connect([this]() { onReplacePaletteClick(); });
    m_setFgBgBtn->Click.connect([this]() { onSetAsFgBgClick(); });
    m_copyHexBtn->Click.connect([this]() { onCopyHexClick(); });
    m_resetBtn->Click.connect([this]() { onResetClick(); });
    m_pickColorBtn->Click.connect([this]() { onPickColorClick(); });

    // Set window size
    setAutoRemap(false);
    setBounds(gfx::Rect(0, 0, 320, 520));

    // Build initial preview and generate
    rebuildPreviewButtons();
    updatePreview();
}

void PaletteGeneratorPopup::rebuildPreviewButtons()
{
    // Clear existing preview buttons
    for (auto* btn : m_previewButtons) {
        m_previewBox->removeChild(btn);
        delete btn;
    }
    m_previewButtons.clear();

    // Create new buttons based on color count
    ColorButtonOptions previewOpts;
    previewOpts.canPinSelector = false;
    previewOpts.showSimpleColors = false;

    for (int i = 0; i < m_settings.colorCount; ++i) {
        auto* btn = new ColorButton(app::Color::fromRgb(128, 128, 128), doc::IMAGE_RGB, previewOpts);
        btn->setExpansive(true);
        m_previewButtons.push_back(btn);
        m_previewBox->addChild(btn);
    }

    m_previewBox->layout();
}

void PaletteGeneratorPopup::onBaseColorChange()
{
    updatePreview();
}

void PaletteGeneratorPopup::onMaterialChange()
{
    m_material = static_cast<tools::PaletteMaterial>(m_materialCombo->getSelectedItemIndex());
    updatePreview();
}

void PaletteGeneratorPopup::onStyleChange()
{
    m_style = static_cast<tools::PaletteStyle>(m_styleCombo->getSelectedItemIndex());
    updatePreview();
}

void PaletteGeneratorPopup::onColorCountChange()
{
    int idx = m_colorCountCombo->getSelectedItemIndex();
    switch (idx) {
        case 0: m_settings.colorCount = 3; break;
        case 1: m_settings.colorCount = 5; break;
        case 2: m_settings.colorCount = 7; break;
        case 3: m_settings.colorCount = 9; break;
        default: m_settings.colorCount = 5; break;
    }
    rebuildPreviewButtons();
    updatePreview();
}

void PaletteGeneratorPopup::onHueShiftChange()
{
    m_settings.hueShift = static_cast<double>(m_hueShiftSlider->getValue());
    m_hueShiftLabel->setText(fmt::format("{:.0f}°", m_settings.hueShift));
    updatePreview();
}

void PaletteGeneratorPopup::onSaturationRangeChange()
{
    m_settings.saturationRange = m_saturationRangeSlider->getValue() / 100.0;
    m_saturationRangeLabel->setText(fmt::format("{:.0f}%", m_settings.saturationRange * 100));
    updatePreview();
}

void PaletteGeneratorPopup::onValueRangeChange()
{
    m_settings.valueRange = m_valueRangeSlider->getValue() / 100.0;
    m_valueRangeLabel->setText(fmt::format("{:.0f}%", m_settings.valueRange * 100));
    updatePreview();
}

void PaletteGeneratorPopup::onHarmonyChange()
{
    m_settings.harmony = static_cast<tools::ColorHarmony>(m_harmonyCombo->getSelectedItemIndex());
    updatePreview();
}

void PaletteGeneratorPopup::onTemperatureChange()
{
    m_settings.temperature = static_cast<tools::TemperatureShift>(m_temperatureCombo->getSelectedItemIndex());
    updatePreview();
}

void PaletteGeneratorPopup::onLightAngleChange()
{
    m_lightAngle = static_cast<double>(m_lightAngleSlider->getValue());
    m_lightAngleLabel->setText(fmt::format("{:.0f}°", m_lightAngle));
    updatePreview();
}

void PaletteGeneratorPopup::onGenerateClick()
{
    updatePreview();
}

void PaletteGeneratorPopup::onResetClick()
{
    // Reset to defaults
    m_settings = tools::PaletteSettings();
    m_material = tools::PaletteMaterial::Matte;
    m_style = tools::PaletteStyle::Natural;
    m_lightAngle = 135.0;

    // Update UI
    m_materialCombo->setSelectedItemIndex(0);
    m_styleCombo->setSelectedItemIndex(0);
    m_colorCountCombo->setSelectedItemIndex(1);  // 5 colors
    m_harmonyCombo->setSelectedItemIndex(0);
    m_temperatureCombo->setSelectedItemIndex(0);
    m_lightAngleSlider->setValue(135);
    m_lightAngleLabel->setText("135°");
    m_hueShiftSlider->setValue(20);
    m_hueShiftLabel->setText("20°");
    m_saturationRangeSlider->setValue(30);
    m_saturationRangeLabel->setText("30%");
    m_valueRangeSlider->setValue(70);
    m_valueRangeLabel->setText("70%");

    rebuildPreviewButtons();
    updatePreview();
}

void PaletteGeneratorPopup::onCopyHexClick()
{
    // Build hex string
    std::string hexStr;
    for (const auto& color : m_currentPalette.colors) {
        if (!hexStr.empty()) hexStr += ", ";
        hexStr += fmt::format("#{:02X}{:02X}{:02X}",
            doc::rgba_getr(color),
            doc::rgba_getg(color),
            doc::rgba_getb(color));
    }

    // Copy to clipboard using system clipboard
    clip::set_text(hexStr);
}

void PaletteGeneratorPopup::updatePreview()
{
    // Get base color
    app::Color baseColor = m_baseColorBtn->getColor();
    doc::color_t docColor = doc::rgba(
        baseColor.getRed(), baseColor.getGreen(), baseColor.getBlue(), 255);

    // Generate palette with full settings
    m_currentPalette = tools::PaletteGenerator::generateWithSettings(
        docColor, m_lightAngle, m_material, m_style, m_settings);

    // Update preview buttons
    for (size_t i = 0; i < m_previewButtons.size() && i < m_currentPalette.colors.size(); ++i) {
        doc::color_t c = m_currentPalette.colors[i];
        m_previewButtons[i]->setColor(app::Color::fromRgb(
            doc::rgba_getr(c),
            doc::rgba_getg(c),
            doc::rgba_getb(c)));
    }
}

void PaletteGeneratorPopup::onAddToPaletteClick()
{
    // Add generated colors to the current sprite's palette
    Context* ctx = App::instance()->context();
    if (!ctx)
        return;

    ContextReader reader(ctx);
    Doc* doc = reader.document();
    if (!doc)
        return;

    doc::Sprite* sprite = reader.sprite();
    if (!sprite)
        return;

    // Get current palette
    const doc::Palette* palette = sprite->palette(reader.frame());
    if (!palette)
        return;

    // Get the colors to add
    const std::vector<doc::color_t>& colors = m_currentPalette.colors;

    // Find insertion point (after last used color or at end)
    int insertIdx = palette->size();

    // Create new palette with added colors
    int newSize = insertIdx + static_cast<int>(colors.size());
    if (newSize > 256) newSize = 256;  // Max palette size

    doc::Palette newPalette(*palette);
    newPalette.resize(newSize);

    // Add colors
    for (size_t i = 0; i < colors.size() && (insertIdx + i) < 256; ++i) {
        newPalette.setEntry(insertIdx + static_cast<int>(i), colors[i]);
    }

    // Create transaction for undo support
    {
        ContextWriter writer(reader);
        Tx tx(writer, "Add Generated Palette", ModifyDocument);
        tx(new cmd::SetPalette(sprite, reader.frame(), &newPalette));
        tx.commit();
    }

    // Refresh palette view
    if (ColorBar::instance())
        ColorBar::instance()->invalidate();
}

void PaletteGeneratorPopup::onSetAsFgBgClick()
{
    if (m_currentPalette.colors.empty())
        return;

    // Set shadow as background, highlight as foreground
    Preferences& pref = Preferences::instance();

    // Set foreground to lightest color
    doc::color_t highlight = m_currentPalette.colors.back();
    pref.colorBar.fgColor(app::Color::fromRgb(
        doc::rgba_getr(highlight),
        doc::rgba_getg(highlight),
        doc::rgba_getb(highlight)));

    // Set background to darkest color
    doc::color_t shadow = m_currentPalette.colors.front();
    pref.colorBar.bgColor(app::Color::fromRgb(
        doc::rgba_getr(shadow),
        doc::rgba_getg(shadow),
        doc::rgba_getb(shadow)));
}

void PaletteGeneratorPopup::onReplacePaletteClick()
{
    // Replace the entire sprite palette with generated colors
    Context* ctx = App::instance()->context();
    if (!ctx)
        return;

    ContextReader reader(ctx);
    Doc* doc = reader.document();
    if (!doc)
        return;

    doc::Sprite* sprite = reader.sprite();
    if (!sprite)
        return;

    // Get the colors to set
    const std::vector<doc::color_t>& colors = m_currentPalette.colors;
    if (colors.empty())
        return;

    // Create new palette with only the generated colors
    doc::Palette newPalette(reader.frame(), static_cast<int>(colors.size()));

    // Set the colors
    for (size_t i = 0; i < colors.size(); ++i) {
        newPalette.setEntry(static_cast<int>(i), colors[i]);
    }

    // Create transaction for undo support
    {
        ContextWriter writer(reader);
        Tx tx(writer, "Replace Palette with Generated Colors", ModifyDocument);
        tx(new cmd::SetPalette(sprite, reader.frame(), &newPalette));
        tx.commit();
    }

    // Refresh palette view
    if (ColorBar::instance())
        ColorBar::instance()->invalidate();
}

void PaletteGeneratorPopup::onPickColorClick()
{
    // Pick color from the current foreground - the user can use the eyedropper
    // tool in the editor to pick from the canvas, then this will sync it
    app::Color fgColor = Preferences::instance().colorBar.fgColor();
    m_baseColorBtn->setColor(fgColor);
    updatePreview();
}

} // namespace app
