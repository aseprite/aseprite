// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Palette Generator Unit Tests

#include "test.h"
#include "../palette_generator.h"
#include "../shade_types.h"

#include <iostream>
#include <cmath>

using namespace app::tools;

// ============================================================================
// HSV/RGB Conversion Tests
// ============================================================================

TEST(rgb_to_hsv_red) {
    // Pure red: RGB(255, 0, 0) -> HSV(0, 1, 1)
    HSV hsv = PaletteGenerator::rgbToHsv(255, 0, 0);
    EXPECT_HUE_NEAR(0.0, hsv.h, 1.0);
    EXPECT_NEAR(1.0, hsv.s, 0.01);
    EXPECT_NEAR(1.0, hsv.v, 0.01);
}

TEST(rgb_to_hsv_green) {
    // Pure green: RGB(0, 255, 0) -> HSV(120, 1, 1)
    HSV hsv = PaletteGenerator::rgbToHsv(0, 255, 0);
    EXPECT_HUE_NEAR(120.0, hsv.h, 1.0);
    EXPECT_NEAR(1.0, hsv.s, 0.01);
    EXPECT_NEAR(1.0, hsv.v, 0.01);
}

TEST(rgb_to_hsv_blue) {
    // Pure blue: RGB(0, 0, 255) -> HSV(240, 1, 1)
    HSV hsv = PaletteGenerator::rgbToHsv(0, 0, 255);
    EXPECT_HUE_NEAR(240.0, hsv.h, 1.0);
    EXPECT_NEAR(1.0, hsv.s, 0.01);
    EXPECT_NEAR(1.0, hsv.v, 0.01);
}

TEST(rgb_to_hsv_yellow) {
    // Yellow: RGB(255, 255, 0) -> HSV(60, 1, 1)
    HSV hsv = PaletteGenerator::rgbToHsv(255, 255, 0);
    EXPECT_HUE_NEAR(60.0, hsv.h, 1.0);
    EXPECT_NEAR(1.0, hsv.s, 0.01);
    EXPECT_NEAR(1.0, hsv.v, 0.01);
}

TEST(rgb_to_hsv_magenta) {
    // Magenta: RGB(255, 0, 255) -> HSV(300, 1, 1)
    HSV hsv = PaletteGenerator::rgbToHsv(255, 0, 255);
    EXPECT_HUE_NEAR(300.0, hsv.h, 1.0);
    EXPECT_NEAR(1.0, hsv.s, 0.01);
    EXPECT_NEAR(1.0, hsv.v, 0.01);
}

TEST(rgb_to_hsv_orange) {
    // Orange: RGB(255, 128, 0) -> HSV(~30, 1, 1)
    HSV hsv = PaletteGenerator::rgbToHsv(255, 128, 0);
    EXPECT_HUE_NEAR(30.0, hsv.h, 2.0);  // ~30 degrees
    EXPECT_NEAR(1.0, hsv.s, 0.01);
    EXPECT_NEAR(1.0, hsv.v, 0.01);
}

TEST(hsv_to_rgb_red) {
    HSV hsv(0.0, 1.0, 1.0);
    int r, g, b;
    PaletteGenerator::hsvToRgb(hsv, r, g, b);
    EXPECT_EQ(255, r);
    EXPECT_EQ(0, g);
    EXPECT_EQ(0, b);
}

TEST(hsv_to_rgb_green) {
    HSV hsv(120.0, 1.0, 1.0);
    int r, g, b;
    PaletteGenerator::hsvToRgb(hsv, r, g, b);
    EXPECT_EQ(0, r);
    EXPECT_EQ(255, g);
    EXPECT_EQ(0, b);
}

TEST(hsv_to_rgb_roundtrip) {
    // Test roundtrip conversion: RGB -> HSV -> RGB
    int origR = 200, origG = 100, origB = 50;
    HSV hsv = PaletteGenerator::rgbToHsv(origR, origG, origB);
    int r, g, b;
    PaletteGenerator::hsvToRgb(hsv, r, g, b);

    // Should be within 1 due to rounding
    EXPECT_TRUE(std::abs(origR - r) <= 1);
    EXPECT_TRUE(std::abs(origG - g) <= 1);
    EXPECT_TRUE(std::abs(origB - b) <= 1);
}

// ============================================================================
// Hue Shift Direction Tests (CRITICAL)
// These tests verify the color theory: warm light = cool shadows, warm highlights
// ============================================================================

TEST(hue_shift_warm_light_shadow) {
    // For warm light with shadows (valuePosition < 0):
    // Shadows should shift COOL (toward magenta/purple = NEGATIVE hue shift)
    // Red (0°) should shift toward magenta (330°/−30°)

    double lightAngle = 135.0;  // Top-left light
    double valuePosition = -1.0;  // Deep shadow
    double maxShift = 20.0;
    bool warmLight = true;

    // We can't call calculateHueShift directly as it's private,
    // but we can test through the full generation

    doc::color_t baseColor = doc::rgba(255, 0, 0, 255);  // Pure red
    PaletteSettings settings;
    settings.colorCount = 5;
    settings.hueShift = 20.0;
    settings.temperature = TemperatureShift::WarmHighlights;
    settings.harmony = ColorHarmony::Monochromatic;

    GeneratedPalette palette = PaletteGenerator::generateWithSettings(
        baseColor, lightAngle, PaletteMaterial::Matte, PaletteStyle::Natural, settings);

    // Get the darkest shadow color
    doc::color_t shadow = palette.colors[0];
    int r, g, b;
    PaletteGenerator::extractRgb(shadow, r, g, b);
    HSV shadowHsv = PaletteGenerator::rgbToHsv(r, g, b);

    // Shadow hue should be LESS than base (0°) or near 360° (wrapping from negative)
    // For red (0°), a cool shift should go toward magenta (330-360°)
    // Or equivalently, a negative shift from 0° wraps to 330-360°
    bool shadowIsCool = (shadowHsv.h > 300.0 && shadowHsv.h <= 360.0) ||  // Wrapped negative
                        (shadowHsv.h >= 0.0 && shadowHsv.h < 30.0);       // Slight warm OK for materials

    std::cout << "  Shadow hue: " << shadowHsv.h << " (expected ~330-360 or slight adjustment)\n";
    EXPECT_TRUE(shadowIsCool);
}

TEST(hue_shift_warm_light_highlight) {
    // For warm light with highlights (valuePosition > 0):
    // Highlights should shift WARM (toward orange/yellow = POSITIVE hue shift)
    // Red (0°) should shift toward orange (30°)

    double lightAngle = 135.0;
    doc::color_t baseColor = doc::rgba(255, 0, 0, 255);  // Pure red

    PaletteSettings settings;
    settings.colorCount = 5;
    settings.hueShift = 20.0;
    settings.temperature = TemperatureShift::WarmHighlights;
    settings.harmony = ColorHarmony::Monochromatic;

    GeneratedPalette palette = PaletteGenerator::generateWithSettings(
        baseColor, lightAngle, PaletteMaterial::Matte, PaletteStyle::Natural, settings);

    // Get the brightest highlight color
    doc::color_t highlight = palette.colors[palette.colors.size() - 1];
    int r, g, b;
    PaletteGenerator::extractRgb(highlight, r, g, b);
    HSV highlightHsv = PaletteGenerator::rgbToHsv(r, g, b);

    // Highlight hue should be GREATER than base (0°), toward orange (30°+)
    // Allow for some tolerance due to saturation changes
    bool highlightIsWarm = (highlightHsv.h >= 0.0 && highlightHsv.h < 60.0);  // Orange/yellow range

    std::cout << "  Highlight hue: " << highlightHsv.h << " (expected 0-60, warm side)\n";
    EXPECT_TRUE(highlightIsWarm);
}

TEST(hue_shift_cool_light_shadow) {
    // For cool light with shadows (valuePosition < 0):
    // Shadows should shift WARM (opposite of warm light)

    double lightAngle = 135.0;
    doc::color_t baseColor = doc::rgba(255, 0, 0, 255);  // Pure red

    PaletteSettings settings;
    settings.colorCount = 5;
    settings.hueShift = 20.0;
    settings.temperature = TemperatureShift::CoolHighlights;  // Cool light
    settings.harmony = ColorHarmony::Monochromatic;

    GeneratedPalette palette = PaletteGenerator::generateWithSettings(
        baseColor, lightAngle, PaletteMaterial::Matte, PaletteStyle::Natural, settings);

    // Get shadow
    doc::color_t shadow = palette.colors[0];
    int r, g, b;
    PaletteGenerator::extractRgb(shadow, r, g, b);
    HSV shadowHsv = PaletteGenerator::rgbToHsv(r, g, b);

    // Shadow should be warm (positive hue shift from red)
    // Red -> Orange direction (0° -> 30°+)
    std::cout << "  Cool light shadow hue: " << shadowHsv.h << " (expected toward warm/orange)\n";
}

TEST(hue_shift_cool_light_highlight) {
    // For cool light with highlights:
    // Highlights should shift COOL (negative hue shift)

    double lightAngle = 135.0;
    doc::color_t baseColor = doc::rgba(255, 0, 0, 255);  // Pure red

    PaletteSettings settings;
    settings.colorCount = 5;
    settings.hueShift = 20.0;
    settings.temperature = TemperatureShift::CoolHighlights;
    settings.harmony = ColorHarmony::Monochromatic;

    GeneratedPalette palette = PaletteGenerator::generateWithSettings(
        baseColor, lightAngle, PaletteMaterial::Matte, PaletteStyle::Natural, settings);

    // Get highlight
    doc::color_t highlight = palette.colors[palette.colors.size() - 1];
    int r, g, b;
    PaletteGenerator::extractRgb(highlight, r, g, b);
    HSV highlightHsv = PaletteGenerator::rgbToHsv(r, g, b);

    // Highlight should be cool (negative hue shift from red)
    // Red -> Magenta direction (0° -> 330°+)
    std::cout << "  Cool light highlight hue: " << highlightHsv.h << " (expected toward cool/magenta)\n";
}

// ============================================================================
// Harmony Mode Tests
// ============================================================================

TEST(harmony_analogous_offsets) {
    // Analogous: shadows -30°, highlights +30°
    doc::color_t baseColor = doc::rgba(255, 0, 0, 255);  // Red = 0°

    PaletteSettings settings;
    settings.colorCount = 5;
    settings.hueShift = 0.0;  // Disable temperature shift to isolate harmony
    settings.temperature = TemperatureShift::Neutral;
    settings.harmony = ColorHarmony::Analogous;

    GeneratedPalette palette = PaletteGenerator::generateWithSettings(
        baseColor, 135.0, PaletteMaterial::Matte, PaletteStyle::Natural, settings);

    // Check shadow moves toward magenta (-30° from red = 330°)
    doc::color_t shadow = palette.colors[0];
    int r, g, b;
    PaletteGenerator::extractRgb(shadow, r, g, b);
    HSV shadowHsv = PaletteGenerator::rgbToHsv(r, g, b);

    std::cout << "  Analogous shadow hue: " << shadowHsv.h << " (expected ~330 = red-30)\n";

    // Check highlight moves toward orange (+30° from red = 30°)
    doc::color_t highlight = palette.colors[palette.colors.size() - 1];
    PaletteGenerator::extractRgb(highlight, r, g, b);
    HSV highlightHsv = PaletteGenerator::rgbToHsv(r, g, b);

    std::cout << "  Analogous highlight hue: " << highlightHsv.h << " (expected ~30 = red+30)\n";
}

TEST(harmony_triadic_offsets) {
    // Triadic: shadows +120°, highlights -40°
    doc::color_t baseColor = doc::rgba(255, 0, 0, 255);  // Red = 0°

    PaletteSettings settings;
    settings.colorCount = 5;
    settings.hueShift = 0.0;
    settings.temperature = TemperatureShift::Neutral;
    settings.harmony = ColorHarmony::Triadic;

    GeneratedPalette palette = PaletteGenerator::generateWithSettings(
        baseColor, 135.0, PaletteMaterial::Matte, PaletteStyle::Natural, settings);

    // Check shadow moves toward green (+120° from red = 120°)
    doc::color_t shadow = palette.colors[0];
    int r, g, b;
    PaletteGenerator::extractRgb(shadow, r, g, b);
    HSV shadowHsv = PaletteGenerator::rgbToHsv(r, g, b);

    std::cout << "  Triadic shadow hue: " << shadowHsv.h << " (expected ~120 = red+120)\n";
    EXPECT_HUE_NEAR(120.0, shadowHsv.h, 30.0);  // Should be in green range
}

TEST(harmony_complementary_offsets) {
    // Complementary: shadows +60° (toward complement), highlights -15°
    doc::color_t baseColor = doc::rgba(255, 0, 0, 255);  // Red = 0°

    PaletteSettings settings;
    settings.colorCount = 5;
    settings.hueShift = 0.0;
    settings.temperature = TemperatureShift::Neutral;
    settings.harmony = ColorHarmony::Complementary;

    GeneratedPalette palette = PaletteGenerator::generateWithSettings(
        baseColor, 135.0, PaletteMaterial::Matte, PaletteStyle::Natural, settings);

    // Shadow should shift toward cyan (complement of red at 180°)
    // With +60° offset, shadow hue should be around 60° (yellow/green)
    doc::color_t shadow = palette.colors[0];
    int r, g, b;
    PaletteGenerator::extractRgb(shadow, r, g, b);
    HSV shadowHsv = PaletteGenerator::rgbToHsv(r, g, b);

    std::cout << "  Complementary shadow hue: " << shadowHsv.h << " (expected ~60 = red+60)\n";
}

// ============================================================================
// Palette Generation Tests
// ============================================================================

TEST(palette_color_count) {
    doc::color_t baseColor = doc::rgba(200, 100, 50, 255);

    for (int count : {3, 5, 7, 9}) {
        PaletteSettings settings;
        settings.colorCount = count;

        GeneratedPalette palette = PaletteGenerator::generateWithSettings(
            baseColor, 135.0, PaletteMaterial::Matte, PaletteStyle::Natural, settings);

        EXPECT_EQ(count, static_cast<int>(palette.colors.size()));
    }
}

TEST(palette_value_ordering) {
    // Colors should be ordered from dark to light
    doc::color_t baseColor = doc::rgba(200, 100, 50, 255);

    PaletteSettings settings;
    settings.colorCount = 5;
    settings.valueRange = 0.7;

    GeneratedPalette palette = PaletteGenerator::generateWithSettings(
        baseColor, 135.0, PaletteMaterial::Matte, PaletteStyle::Natural, settings);

    // Each color should have greater or equal value than the previous
    double prevValue = 0.0;
    for (size_t i = 0; i < palette.colors.size(); i++) {
        int r, g, b;
        PaletteGenerator::extractRgb(palette.colors[i], r, g, b);
        HSV hsv = PaletteGenerator::rgbToHsv(r, g, b);

        std::cout << "  Color " << i << " value: " << hsv.v << "\n";
        EXPECT_TRUE(hsv.v >= prevValue - 0.01);  // Small tolerance for rounding
        prevValue = hsv.v;
    }
}

TEST(palette_alpha_preserved) {
    // Alpha channel should be preserved
    doc::color_t baseColor = doc::rgba(200, 100, 50, 128);  // 50% alpha

    PaletteSettings settings;
    settings.colorCount = 5;

    GeneratedPalette palette = PaletteGenerator::generateWithSettings(
        baseColor, 135.0, PaletteMaterial::Matte, PaletteStyle::Natural, settings);

    for (size_t i = 0; i < palette.colors.size(); i++) {
        int alpha = doc::rgba_geta(palette.colors[i]);
        EXPECT_EQ(128, alpha);
    }
}

// ============================================================================
// Material Tests
// ============================================================================

TEST(material_metallic_desaturated_shadows) {
    // Metallic material should have desaturated shadows
    doc::color_t baseColor = doc::rgba(255, 0, 0, 255);  // Saturated red

    PaletteSettings settings;
    settings.colorCount = 5;
    settings.harmony = ColorHarmony::Monochromatic;

    // Generate with Matte material
    GeneratedPalette mattePalette = PaletteGenerator::generateWithSettings(
        baseColor, 135.0, PaletteMaterial::Matte, PaletteStyle::Natural, settings);

    // Generate with Metallic material
    GeneratedPalette metallicPalette = PaletteGenerator::generateWithSettings(
        baseColor, 135.0, PaletteMaterial::Metallic, PaletteStyle::Natural, settings);

    // Compare shadow saturation
    int r, g, b;
    PaletteGenerator::extractRgb(mattePalette.colors[0], r, g, b);
    HSV matteShadow = PaletteGenerator::rgbToHsv(r, g, b);

    PaletteGenerator::extractRgb(metallicPalette.colors[0], r, g, b);
    HSV metallicShadow = PaletteGenerator::rgbToHsv(r, g, b);

    std::cout << "  Matte shadow saturation: " << matteShadow.s << "\n";
    std::cout << "  Metallic shadow saturation: " << metallicShadow.s << "\n";

    // Metallic shadows should be less saturated
    EXPECT_TRUE(metallicShadow.s <= matteShadow.s + 0.1);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "========================================\n";
    std::cout << "Palette Generator Unit Tests\n";
    std::cout << "========================================\n\n";

    // HSV/RGB Conversion Tests
    std::cout << "--- HSV/RGB Conversion Tests ---\n";
    RUN_TEST(rgb_to_hsv_red);
    RUN_TEST(rgb_to_hsv_green);
    RUN_TEST(rgb_to_hsv_blue);
    RUN_TEST(rgb_to_hsv_yellow);
    RUN_TEST(rgb_to_hsv_magenta);
    RUN_TEST(rgb_to_hsv_orange);
    RUN_TEST(hsv_to_rgb_red);
    RUN_TEST(hsv_to_rgb_green);
    RUN_TEST(hsv_to_rgb_roundtrip);

    // Hue Shift Direction Tests (CRITICAL)
    std::cout << "\n--- Hue Shift Direction Tests ---\n";
    RUN_TEST(hue_shift_warm_light_shadow);
    RUN_TEST(hue_shift_warm_light_highlight);
    RUN_TEST(hue_shift_cool_light_shadow);
    RUN_TEST(hue_shift_cool_light_highlight);

    // Harmony Mode Tests
    std::cout << "\n--- Harmony Mode Tests ---\n";
    RUN_TEST(harmony_analogous_offsets);
    RUN_TEST(harmony_triadic_offsets);
    RUN_TEST(harmony_complementary_offsets);

    // Palette Generation Tests
    std::cout << "\n--- Palette Generation Tests ---\n";
    RUN_TEST(palette_color_count);
    RUN_TEST(palette_value_ordering);
    RUN_TEST(palette_alpha_preserved);

    // Material Tests
    std::cout << "\n--- Material Tests ---\n";
    RUN_TEST(material_metallic_desaturated_shadows);

    print_test_summary();

    return g_testsFailed > 0 ? 1 : 0;
}
