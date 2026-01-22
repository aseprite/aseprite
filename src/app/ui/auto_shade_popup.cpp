// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Options Popup Window

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/auto_shade_popup.h"

#include "app/app.h"
#include "app/color.h"
#include "app/ui/skin/skin_theme.h"
#include "doc/image.h"
#include "ui/box.h"
#include "ui/grid.h"
#include "ui/separator.h"
#include "ui/view.h"

#include "fmt/format.h"

namespace app {

using namespace ui;
using namespace skin;

AutoShadePopup::AutoShadePopup()
    : PopupWindow("Auto-Shade Options",
                  ClickBehavior::DoNothingOnClick,
                  EnterBehavior::DoNothingOnEnter,
                  true)  // withCloseButton
{
    // Make the window floating (movable)
    makeFloating();

    auto theme = SkinTheme::get(this);

    // Main vertical box
    auto mainBox = new VBox();
    mainBox->setExpansive(true);

    // ---- Light Settings Section ----
    auto lightSection = new VBox();
    lightSection->addChild(new Label("Light Settings"));
    lightSection->addChild(new Separator("", HORIZONTAL));

    // Light angle with flip buttons
    auto lightAngleBox = new HBox();
    lightAngleBox->addChild(new Label("Angle:"));
    m_lightAngleSlider = new Slider(0, 360, static_cast<int>(m_config.lightAngle));
    m_lightAngleSlider->setExpansive(true);
    m_lightAngleLabel = new Label(fmt::format("{:.0f}", m_config.lightAngle));
    m_lightAngleLabel->setMinSize(gfx::Size(30, 0));
    lightAngleBox->addChild(m_lightAngleSlider);
    lightAngleBox->addChild(m_lightAngleLabel);
    lightSection->addChild(lightAngleBox);

    // Flip buttons for quick light direction control
    auto flipButtonsBox = new HBox();
    flipButtonsBox->addChild(new Label("Flip:"));
    m_flipHButton = new Button("H");
    m_flipHButton->setMinSize(gfx::Size(28, 0));
    m_flipVButton = new Button("V");
    m_flipVButton->setMinSize(gfx::Size(28, 0));
    flipButtonsBox->addChild(m_flipHButton);
    flipButtonsBox->addChild(m_flipVButton);
    flipButtonsBox->addChild(new BoxFiller());  // Push buttons to left
    lightSection->addChild(flipButtonsBox);

    // Ambient level
    auto ambientBox = new HBox();
    ambientBox->addChild(new Label("Ambient:"));
    m_ambientSlider = new Slider(0, 100, static_cast<int>(m_config.ambientLevel * 100));
    m_ambientSlider->setExpansive(true);
    m_ambientLabel = new Label(fmt::format("{:.0f}%", m_config.ambientLevel * 100));
    m_ambientLabel->setMinSize(gfx::Size(35, 0));
    ambientBox->addChild(m_ambientSlider);
    ambientBox->addChild(m_ambientLabel);
    lightSection->addChild(ambientBox);

    // Light elevation (angle from horizontal: 0=front, 90=top, 180=back)
    auto elevationBox = new HBox();
    elevationBox->addChild(new Label("Elevation:"));
    m_lightElevationSlider = new Slider(0, 180, static_cast<int>(m_config.lightElevation));
    m_lightElevationSlider->setExpansive(true);
    m_lightElevationLabel = new Label(fmt::format("{:.0f}", m_config.lightElevation));
    m_lightElevationLabel->setMinSize(gfx::Size(35, 0));
    elevationBox->addChild(m_lightElevationSlider);
    elevationBox->addChild(m_lightElevationLabel);
    lightSection->addChild(elevationBox);

    // Roundness (sphere curvature)
    auto roundnessBox = new HBox();
    roundnessBox->addChild(new Label("Roundness:"));
    m_roundnessSlider = new Slider(0, 200, static_cast<int>(m_config.roundness * 100));
    m_roundnessSlider->setExpansive(true);
    m_roundnessLabel = new Label(fmt::format("{:.1f}", m_config.roundness));
    m_roundnessLabel->setMinSize(gfx::Size(35, 0));
    roundnessBox->addChild(m_roundnessSlider);
    roundnessBox->addChild(m_roundnessLabel);
    lightSection->addChild(roundnessBox);

    // Highlight focus (how concentrated the highlight is)
    auto highlightFocusBox = new HBox();
    highlightFocusBox->addChild(new Label("Hi Focus:"));
    m_highlightFocusSlider = new Slider(0, 100, static_cast<int>(m_config.highlightFocus * 100));
    m_highlightFocusSlider->setExpansive(true);
    m_highlightFocusLabel = new Label(fmt::format("{:.0f}%", m_config.highlightFocus * 100));
    m_highlightFocusLabel->setMinSize(gfx::Size(35, 0));
    highlightFocusBox->addChild(m_highlightFocusSlider);
    highlightFocusBox->addChild(m_highlightFocusLabel);
    lightSection->addChild(highlightFocusBox);

    mainBox->addChild(lightSection);

    // ---- Colors Section ----
    auto colorsSection = new VBox();
    colorsSection->addChild(new Label("Colors"));
    colorsSection->addChild(new Separator("", HORIZONTAL));

    ColorButtonOptions colorOpts;
    colorOpts.canPinSelector = false;
    colorOpts.showSimpleColors = false;
    colorOpts.showIndexTab = true;

    auto colorsGrid = new Grid(2, false);

    colorsGrid->addChildInCell(new Label("Shadow:"), 1, 1, 0);
    m_shadowColorBtn = new ColorButton(
        app::Color::fromRgb(
            doc::rgba_getr(m_config.shadowColor),
            doc::rgba_getg(m_config.shadowColor),
            doc::rgba_getb(m_config.shadowColor)),
        doc::IMAGE_RGB, colorOpts);
    m_shadowColorBtn->setExpansive(true);
    colorsGrid->addChildInCell(m_shadowColorBtn, 1, 1, 0);

    colorsGrid->addChildInCell(new Label("Base:"), 1, 1, 0);
    m_baseColorBtn = new ColorButton(
        app::Color::fromRgb(
            doc::rgba_getr(m_config.baseColor),
            doc::rgba_getg(m_config.baseColor),
            doc::rgba_getb(m_config.baseColor)),
        doc::IMAGE_RGB, colorOpts);
    m_baseColorBtn->setExpansive(true);
    colorsGrid->addChildInCell(m_baseColorBtn, 1, 1, 0);

    colorsGrid->addChildInCell(new Label("Highlight:"), 1, 1, 0);
    m_highlightColorBtn = new ColorButton(
        app::Color::fromRgb(
            doc::rgba_getr(m_config.highlightColor),
            doc::rgba_getg(m_config.highlightColor),
            doc::rgba_getb(m_config.highlightColor)),
        doc::IMAGE_RGB, colorOpts);
    m_highlightColorBtn->setExpansive(true);
    colorsGrid->addChildInCell(m_highlightColorBtn, 1, 1, 0);

    colorsSection->addChild(colorsGrid);
    mainBox->addChild(colorsSection);

    // ---- Shading Options Section ----
    auto optionsSection = new VBox();
    optionsSection->addChild(new Label("Shading Options"));
    optionsSection->addChild(new Separator("", HORIZONTAL));

    auto optionsGrid = new Grid(2, false);

    // Shading mode
    optionsGrid->addChildInCell(new Label("Mode:"), 1, 1, 0);
    m_shadingModeCombo = new ComboBox();
    m_shadingModeCombo->addItem("3-Shade");
    m_shadingModeCombo->addItem("5-Shade");
    m_shadingModeCombo->addItem("Gradient");
    m_shadingModeCombo->setSelectedItemIndex(static_cast<int>(m_config.shadingMode));
    m_shadingModeCombo->setExpansive(true);
    optionsGrid->addChildInCell(m_shadingModeCombo, 1, 1, 0);

    // Normal method
    optionsGrid->addChildInCell(new Label("Normals:"), 1, 1, 0);
    m_normalMethodCombo = new ComboBox();
    m_normalMethodCombo->addItem("Radial");
    m_normalMethodCombo->addItem("Gradient");
    m_normalMethodCombo->addItem("Sobel");
    m_normalMethodCombo->addItem("Contour");
    m_normalMethodCombo->setSelectedItemIndex(static_cast<int>(m_config.normalMethod));
    m_normalMethodCombo->setExpansive(true);
    optionsGrid->addChildInCell(m_normalMethodCombo, 1, 1, 0);

    // Shape type (for normal calculation)
    optionsGrid->addChildInCell(new Label("Shape:"), 1, 1, 0);
    m_shapeTypeCombo = new ComboBox();
    m_shapeTypeCombo->addItem("Sphere");         // Smooth spherical
    m_shapeTypeCombo->addItem("Adaptive");       // Follow shape silhouette
    m_shapeTypeCombo->addItem("Cylinder");       // Cylindrical (vertical axis)
    m_shapeTypeCombo->addItem("Flat");           // Uniform flat
    m_shapeTypeCombo->setSelectedItemIndex(static_cast<int>(m_config.shapeType));
    m_shapeTypeCombo->setExpansive(true);
    optionsGrid->addChildInCell(m_shapeTypeCombo, 1, 1, 0);

    // Fill mode
    optionsGrid->addChildInCell(new Label("Fill:"), 1, 1, 0);
    m_fillModeCombo = new ComboBox();
    m_fillModeCombo->addItem("Same Color");
    m_fillModeCombo->addItem("All Non-Transparent");
    m_fillModeCombo->addItem("Bounded Area");
    m_fillModeCombo->setSelectedItemIndex(static_cast<int>(m_config.fillMode));
    m_fillModeCombo->setExpansive(true);
    optionsGrid->addChildInCell(m_fillModeCombo, 1, 1, 0);

    // Color source (foreground/background)
    optionsGrid->addChildInCell(new Label("Color from:"), 1, 1, 0);
    m_colorSourceCombo = new ComboBox();
    m_colorSourceCombo->addItem("Foreground");
    m_colorSourceCombo->addItem("Background");
    m_colorSourceCombo->setSelectedItemIndex(static_cast<int>(m_config.colorSource));
    m_colorSourceCombo->setExpansive(true);
    optionsGrid->addChildInCell(m_colorSourceCombo, 1, 1, 0);

    // Shading style (how colors are arranged/applied)
    optionsGrid->addChildInCell(new Label("Style:"), 1, 1, 0);
    m_shadingStyleCombo = new ComboBox();
    m_shadingStyleCombo->addItem("Classic Cartoon");   // Hard edges, solid zones
    m_shadingStyleCombo->addItem("Soft Cartoon");      // Softer edges, AA transitions
    m_shadingStyleCombo->addItem("Oil Soft");          // Painterly, irregular texture
    m_shadingStyleCombo->addItem("Raw Paint");         // Smooth dithered gradients
    m_shadingStyleCombo->addItem("Dotted");            // Screen-tone dot pattern
    m_shadingStyleCombo->addItem("Stroke Sphere");     // Concentric circular strokes
    m_shadingStyleCombo->addItem("Stroke Vertical");   // Vertical hatching lines
    m_shadingStyleCombo->addItem("Stroke Horizontal"); // Horizontal hatching lines
    m_shadingStyleCombo->addItem("Small Grain");       // Fine noise texture
    m_shadingStyleCombo->addItem("Large Grain");       // Chunky noise clusters
    m_shadingStyleCombo->addItem("Tricky Shading");    // Complex mixed patterns
    m_shadingStyleCombo->addItem("Soft Pattern");      // Perlin noise overlay
    m_shadingStyleCombo->addItem("Wrinkled");          // Flow-based wrinkle lines
    m_shadingStyleCombo->addItem("Patterned");         // Regular geometric patterns
    m_shadingStyleCombo->addItem("Wood");              // Wood grain rings/lines
    m_shadingStyleCombo->addItem("Hard Brush");        // Visible brush strokes
    m_shadingStyleCombo->setSelectedItemIndex(static_cast<int>(m_config.shadingStyle));
    m_shadingStyleCombo->setExpansive(true);
    optionsGrid->addChildInCell(m_shadingStyleCombo, 1, 1, 0);

    optionsSection->addChild(optionsGrid);

    // Show outline checkbox
    m_showOutlineCheck = new CheckBox("Show outline in preview");
    m_showOutlineCheck->setSelected(m_config.showOutline);
    optionsSection->addChild(m_showOutlineCheck);

    // Selective outline checkbox
    m_selectiveOutlineCheck = new CheckBox("Selective outline (light/shadow)");
    m_selectiveOutlineCheck->setSelected(m_config.enableSelectiveOutline);
    optionsSection->addChild(m_selectiveOutlineCheck);

    // Outline colors grid
    auto outlineColorsGrid = new Grid(2, false);

    outlineColorsGrid->addChildInCell(new Label("Light outline:"), 1, 1, 0);
    m_lightOutlineColorBtn = new ColorButton(
        app::Color::fromRgb(
            doc::rgba_getr(m_config.lightOutlineColor),
            doc::rgba_getg(m_config.lightOutlineColor),
            doc::rgba_getb(m_config.lightOutlineColor)),
        doc::IMAGE_RGB, colorOpts);
    m_lightOutlineColorBtn->setExpansive(true);
    outlineColorsGrid->addChildInCell(m_lightOutlineColorBtn, 1, 1, 0);

    outlineColorsGrid->addChildInCell(new Label("Shadow outline:"), 1, 1, 0);
    m_shadowOutlineColorBtn = new ColorButton(
        app::Color::fromRgb(
            doc::rgba_getr(m_config.shadowOutlineColor),
            doc::rgba_getg(m_config.shadowOutlineColor),
            doc::rgba_getb(m_config.shadowOutlineColor)),
        doc::IMAGE_RGB, colorOpts);
    m_shadowOutlineColorBtn->setExpansive(true);
    outlineColorsGrid->addChildInCell(m_shadowOutlineColorBtn, 1, 1, 0);

    optionsSection->addChild(outlineColorsGrid);

    mainBox->addChild(optionsSection);

    // ---- Advanced Options Section ----
    auto advancedSection = new VBox();
    advancedSection->addChild(new Label("Advanced"));
    advancedSection->addChild(new Separator("", HORIZONTAL));

    // Anti-aliasing checkbox
    m_antiAliasingCheck = new CheckBox("Anti-aliasing (silhouette)");
    m_antiAliasingCheck->setSelected(m_config.enableAntiAliasing);
    advancedSection->addChild(m_antiAliasingCheck);

    // Dithering checkbox
    m_ditheringCheck = new CheckBox("Dithering (band transitions)");
    m_ditheringCheck->setSelected(m_config.enableDithering);
    advancedSection->addChild(m_ditheringCheck);

    mainBox->addChild(advancedSection);

    // Wrap mainBox in a scrollable view
    m_view = new View();
    m_view->attachToView(mainBox);
    m_view->setExpansive(true);
    addChild(m_view);

    // Connect signals
    m_lightAngleSlider->Change.connect([this]() { onLightAngleChange(); });
    m_flipHButton->Click.connect([this]() { onFlipHorizontal(); });
    m_flipVButton->Click.connect([this]() { onFlipVertical(); });
    m_ambientSlider->Change.connect([this]() { onAmbientChange(); });
    m_lightElevationSlider->Change.connect([this]() { onLightElevationChange(); });
    m_roundnessSlider->Change.connect([this]() { onRoundnessChange(); });
    m_highlightFocusSlider->Change.connect([this]() { onHighlightFocusChange(); });
    m_shadingModeCombo->Change.connect([this]() { onShadingModeChange(); });
    m_normalMethodCombo->Change.connect([this]() { onNormalMethodChange(); });
    m_shapeTypeCombo->Change.connect([this]() { onShapeTypeChange(); });
    m_fillModeCombo->Change.connect([this]() { onFillModeChange(); });
    m_colorSourceCombo->Change.connect([this]() { onColorSourceChange(); });
    m_shadingStyleCombo->Change.connect([this]() { onShadingStyleChange(); });
    m_shadowColorBtn->Change.connect([this]() { onColorChange(); });
    m_baseColorBtn->Change.connect([this]() { onColorChange(); });
    m_highlightColorBtn->Change.connect([this]() { onColorChange(); });
    m_showOutlineCheck->Click.connect([this]() { onShowOutlineChange(); });
    m_selectiveOutlineCheck->Click.connect([this]() { onSelectiveOutlineChange(); });
    m_lightOutlineColorBtn->Change.connect([this]() { onOutlineColorChange(); });
    m_shadowOutlineColorBtn->Change.connect([this]() { onOutlineColorChange(); });
    m_antiAliasingCheck->Click.connect([this]() { onAntiAliasingChange(); });
    m_ditheringCheck->Click.connect([this]() { onDitheringChange(); });

    // Set initial size (scrollable, so can be smaller)
    setAutoRemap(false);
    setBounds(gfx::Rect(0, 0, 280, 450));
}

tools::ShadeConfig AutoShadePopup::getConfig() const
{
    return m_config;
}

void AutoShadePopup::setConfig(const tools::ShadeConfig& config)
{
    // Guard against feedback loops
    m_updatingFromCode = true;

    m_config = config;

    // Update UI from config
    m_lightAngleSlider->setValue(static_cast<int>(config.lightAngle));
    m_lightAngleLabel->setText(fmt::format("{:.0f}", config.lightAngle));

    m_ambientSlider->setValue(static_cast<int>(config.ambientLevel * 100));
    m_ambientLabel->setText(fmt::format("{:.0f}%", config.ambientLevel * 100));

    m_lightElevationSlider->setValue(static_cast<int>(config.lightElevation));
    m_lightElevationLabel->setText(fmt::format("{:.0f}", config.lightElevation));

    m_roundnessSlider->setValue(static_cast<int>(config.roundness * 100));
    m_roundnessLabel->setText(fmt::format("{:.1f}", config.roundness));

    m_shadingModeCombo->setSelectedItemIndex(static_cast<int>(config.shadingMode));
    m_normalMethodCombo->setSelectedItemIndex(static_cast<int>(config.normalMethod));
    m_shapeTypeCombo->setSelectedItemIndex(static_cast<int>(config.shapeType));
    m_fillModeCombo->setSelectedItemIndex(static_cast<int>(config.fillMode));
    m_colorSourceCombo->setSelectedItemIndex(static_cast<int>(config.colorSource));
    m_shadingStyleCombo->setSelectedItemIndex(static_cast<int>(config.shadingStyle));

    m_shadowColorBtn->setColor(app::Color::fromRgb(
        doc::rgba_getr(config.shadowColor),
        doc::rgba_getg(config.shadowColor),
        doc::rgba_getb(config.shadowColor)));

    m_baseColorBtn->setColor(app::Color::fromRgb(
        doc::rgba_getr(config.baseColor),
        doc::rgba_getg(config.baseColor),
        doc::rgba_getb(config.baseColor)));

    m_highlightColorBtn->setColor(app::Color::fromRgb(
        doc::rgba_getr(config.highlightColor),
        doc::rgba_getg(config.highlightColor),
        doc::rgba_getb(config.highlightColor)));

    m_showOutlineCheck->setSelected(config.showOutline);

    m_highlightFocusSlider->setValue(static_cast<int>(config.highlightFocus * 100));
    m_highlightFocusLabel->setText(fmt::format("{:.0f}%", config.highlightFocus * 100));

    m_selectiveOutlineCheck->setSelected(config.enableSelectiveOutline);

    m_lightOutlineColorBtn->setColor(app::Color::fromRgb(
        doc::rgba_getr(config.lightOutlineColor),
        doc::rgba_getg(config.lightOutlineColor),
        doc::rgba_getb(config.lightOutlineColor)));

    m_shadowOutlineColorBtn->setColor(app::Color::fromRgb(
        doc::rgba_getr(config.shadowOutlineColor),
        doc::rgba_getg(config.shadowOutlineColor),
        doc::rgba_getb(config.shadowOutlineColor)));

    m_antiAliasingCheck->setSelected(config.enableAntiAliasing);
    m_ditheringCheck->setSelected(config.enableDithering);

    m_updatingFromCode = false;
}

void AutoShadePopup::onLightAngleChange()
{
    m_config.lightAngle = static_cast<double>(m_lightAngleSlider->getValue());
    m_lightAngleLabel->setText(fmt::format("{:.0f}", m_config.lightAngle));
    notifyChange();
}

void AutoShadePopup::onFlipHorizontal()
{
    // Flip horizontally: mirror across vertical axis (180 - angle)
    double newAngle = 180.0 - m_config.lightAngle;
    if (newAngle < 0) newAngle += 360.0;
    if (newAngle >= 360) newAngle -= 360.0;
    m_config.lightAngle = newAngle;
    m_lightAngleSlider->setValue(static_cast<int>(newAngle));
    m_lightAngleLabel->setText(fmt::format("{:.0f}", newAngle));
    notifyChange();
}

void AutoShadePopup::onFlipVertical()
{
    // Flip vertically: mirror across horizontal axis (360 - angle or -angle)
    double newAngle = 360.0 - m_config.lightAngle;
    if (newAngle >= 360) newAngle -= 360.0;
    m_config.lightAngle = newAngle;
    m_lightAngleSlider->setValue(static_cast<int>(newAngle));
    m_lightAngleLabel->setText(fmt::format("{:.0f}", newAngle));
    notifyChange();
}

void AutoShadePopup::onAmbientChange()
{
    m_config.ambientLevel = m_ambientSlider->getValue() / 100.0;
    m_ambientLabel->setText(fmt::format("{:.0f}%", m_config.ambientLevel * 100));
    notifyChange();
}

void AutoShadePopup::onLightElevationChange()
{
    m_config.lightElevation = static_cast<double>(m_lightElevationSlider->getValue());
    m_lightElevationLabel->setText(fmt::format("{:.0f}", m_config.lightElevation));
    notifyChange();
}

void AutoShadePopup::onRoundnessChange()
{
    m_config.roundness = m_roundnessSlider->getValue() / 100.0;
    m_roundnessLabel->setText(fmt::format("{:.1f}", m_config.roundness));
    notifyChange();
}

void AutoShadePopup::onShadingModeChange()
{
    m_config.shadingMode = static_cast<tools::ShadingMode>(
        m_shadingModeCombo->getSelectedItemIndex());
    notifyChange();
}

void AutoShadePopup::onNormalMethodChange()
{
    m_config.normalMethod = static_cast<tools::NormalMethod>(
        m_normalMethodCombo->getSelectedItemIndex());
    notifyChange();
}

void AutoShadePopup::onShapeTypeChange()
{
    m_config.shapeType = static_cast<tools::ShapeType>(
        m_shapeTypeCombo->getSelectedItemIndex());
    notifyChange();
}

void AutoShadePopup::onFillModeChange()
{
    m_config.fillMode = static_cast<tools::FillMode>(
        m_fillModeCombo->getSelectedItemIndex());
    notifyChange();
}

void AutoShadePopup::onColorSourceChange()
{
    m_config.colorSource = static_cast<tools::ColorSource>(
        m_colorSourceCombo->getSelectedItemIndex());
    notifyChange();
}

void AutoShadePopup::onShadingStyleChange()
{
    m_config.shadingStyle = static_cast<tools::ShadingStyle>(
        m_shadingStyleCombo->getSelectedItemIndex());
    notifyChange();
}

void AutoShadePopup::onColorChange()
{
    app::Color shadowColor = m_shadowColorBtn->getColor();
    app::Color baseColor = m_baseColorBtn->getColor();
    app::Color highlightColor = m_highlightColorBtn->getColor();

    m_config.shadowColor = doc::rgba(
        shadowColor.getRed(), shadowColor.getGreen(), shadowColor.getBlue(), 255);
    m_config.baseColor = doc::rgba(
        baseColor.getRed(), baseColor.getGreen(), baseColor.getBlue(), 255);
    m_config.highlightColor = doc::rgba(
        highlightColor.getRed(), highlightColor.getGreen(), highlightColor.getBlue(), 255);

    notifyChange();
}

void AutoShadePopup::onShowOutlineChange()
{
    m_config.showOutline = m_showOutlineCheck->isSelected();
    notifyChange();
}

void AutoShadePopup::onHighlightFocusChange()
{
    m_config.highlightFocus = m_highlightFocusSlider->getValue() / 100.0;
    m_highlightFocusLabel->setText(fmt::format("{:.0f}%", m_config.highlightFocus * 100));
    notifyChange();
}

void AutoShadePopup::onSelectiveOutlineChange()
{
    m_config.enableSelectiveOutline = m_selectiveOutlineCheck->isSelected();
    notifyChange();
}

void AutoShadePopup::onOutlineColorChange()
{
    app::Color lightColor = m_lightOutlineColorBtn->getColor();
    app::Color shadowColor = m_shadowOutlineColorBtn->getColor();

    m_config.lightOutlineColor = doc::rgba(
        lightColor.getRed(), lightColor.getGreen(), lightColor.getBlue(), 255);
    m_config.shadowOutlineColor = doc::rgba(
        shadowColor.getRed(), shadowColor.getGreen(), shadowColor.getBlue(), 255);

    notifyChange();
}

void AutoShadePopup::onAntiAliasingChange()
{
    m_config.enableAntiAliasing = m_antiAliasingCheck->isSelected();
    notifyChange();
}

void AutoShadePopup::onDitheringChange()
{
    m_config.enableDithering = m_ditheringCheck->isSelected();
    notifyChange();
}

void AutoShadePopup::notifyChange()
{
    // Don't emit signal if we're updating from code (prevents feedback loop)
    if (m_updatingFromCode)
        return;

    ConfigChange(m_config);
}

} // namespace app
