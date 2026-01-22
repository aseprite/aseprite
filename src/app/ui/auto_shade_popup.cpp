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
#include "ui/grid.h"
#include "ui/separator.h"

#include "fmt/format.h"

namespace app {

using namespace ui;
using namespace skin;

AutoShadePopup::AutoShadePopup()
    : PopupWindow("Auto-Shade Options",
                  ClickBehavior::CloseOnClickInOtherWindow,
                  EnterBehavior::DoNothingOnEnter)
{
    auto theme = SkinTheme::get(this);

    // Main vertical box
    auto mainBox = new VBox();
    mainBox->setExpansive(true);

    // ---- Light Settings Section ----
    auto lightSection = new VBox();
    lightSection->addChild(new Label("Light Settings"));
    lightSection->addChild(new Separator("", HORIZONTAL));

    // Light angle
    auto lightAngleBox = new HBox();
    lightAngleBox->addChild(new Label("Angle:"));
    m_lightAngleSlider = new Slider(0, 360, static_cast<int>(m_config.lightAngle));
    m_lightAngleSlider->setExpansive(true);
    m_lightAngleLabel = new Label(fmt::format("{:.0f}", m_config.lightAngle));
    m_lightAngleLabel->setMinSize(gfx::Size(30, 0));
    lightAngleBox->addChild(m_lightAngleSlider);
    lightAngleBox->addChild(m_lightAngleLabel);
    lightSection->addChild(lightAngleBox);

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

    // Light elevation (Z component - how much from above)
    auto elevationBox = new HBox();
    elevationBox->addChild(new Label("Elevation:"));
    m_lightElevationSlider = new Slider(0, 100, static_cast<int>(m_config.lightElevation * 100));
    m_lightElevationSlider->setExpansive(true);
    m_lightElevationLabel = new Label(fmt::format("{:.0f}%", m_config.lightElevation * 100));
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

    // Fill mode
    optionsGrid->addChildInCell(new Label("Fill:"), 1, 1, 0);
    m_fillModeCombo = new ComboBox();
    m_fillModeCombo->addItem("Same Color");
    m_fillModeCombo->addItem("All Non-Transparent");
    m_fillModeCombo->addItem("Bounded Area");
    m_fillModeCombo->setSelectedItemIndex(static_cast<int>(m_config.fillMode));
    m_fillModeCombo->setExpansive(true);
    optionsGrid->addChildInCell(m_fillModeCombo, 1, 1, 0);

    optionsSection->addChild(optionsGrid);

    // Show outline checkbox
    m_showOutlineCheck = new CheckBox("Show outline in preview");
    m_showOutlineCheck->setSelected(m_config.showOutline);
    optionsSection->addChild(m_showOutlineCheck);

    mainBox->addChild(optionsSection);

    addChild(mainBox);

    // Connect signals
    m_lightAngleSlider->Change.connect([this]() { onLightAngleChange(); });
    m_ambientSlider->Change.connect([this]() { onAmbientChange(); });
    m_lightElevationSlider->Change.connect([this]() { onLightElevationChange(); });
    m_roundnessSlider->Change.connect([this]() { onRoundnessChange(); });
    m_shadingModeCombo->Change.connect([this]() { onShadingModeChange(); });
    m_normalMethodCombo->Change.connect([this]() { onNormalMethodChange(); });
    m_fillModeCombo->Change.connect([this]() { onFillModeChange(); });
    m_shadowColorBtn->Change.connect([this]() { onColorChange(); });
    m_baseColorBtn->Change.connect([this]() { onColorChange(); });
    m_highlightColorBtn->Change.connect([this]() { onColorChange(); });
    m_showOutlineCheck->Click.connect([this]() { onShowOutlineChange(); });

    // Set initial size (increased for new controls)
    setAutoRemap(false);
    setBounds(gfx::Rect(0, 0, 260, 420));
}

tools::ShadeConfig AutoShadePopup::getConfig() const
{
    return m_config;
}

void AutoShadePopup::setConfig(const tools::ShadeConfig& config)
{
    m_config = config;

    // Update UI from config
    m_lightAngleSlider->setValue(static_cast<int>(config.lightAngle));
    m_lightAngleLabel->setText(fmt::format("{:.0f}", config.lightAngle));

    m_ambientSlider->setValue(static_cast<int>(config.ambientLevel * 100));
    m_ambientLabel->setText(fmt::format("{:.0f}%", config.ambientLevel * 100));

    m_lightElevationSlider->setValue(static_cast<int>(config.lightElevation * 100));
    m_lightElevationLabel->setText(fmt::format("{:.0f}%", config.lightElevation * 100));

    m_roundnessSlider->setValue(static_cast<int>(config.roundness * 100));
    m_roundnessLabel->setText(fmt::format("{:.1f}", config.roundness));

    m_shadingModeCombo->setSelectedItemIndex(static_cast<int>(config.shadingMode));
    m_normalMethodCombo->setSelectedItemIndex(static_cast<int>(config.normalMethod));
    m_fillModeCombo->setSelectedItemIndex(static_cast<int>(config.fillMode));

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
}

void AutoShadePopup::onLightAngleChange()
{
    m_config.lightAngle = static_cast<double>(m_lightAngleSlider->getValue());
    m_lightAngleLabel->setText(fmt::format("{:.0f}", m_config.lightAngle));
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
    m_config.lightElevation = m_lightElevationSlider->getValue() / 100.0;
    m_lightElevationLabel->setText(fmt::format("{:.0f}%", m_config.lightElevation * 100));
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

void AutoShadePopup::onFillModeChange()
{
    m_config.fillMode = static_cast<tools::FillMode>(
        m_fillModeCombo->getSelectedItemIndex());
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

void AutoShadePopup::notifyChange()
{
    ConfigChange(m_config);
}

} // namespace app
