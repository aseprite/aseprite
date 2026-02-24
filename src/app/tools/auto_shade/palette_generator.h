// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Palette Generator
//
// Generates a complete shading palette from a single base color,
// light angle, material type, and style preference using color theory.

#ifndef APP_TOOLS_AUTO_SHADE_PALETTE_GENERATOR_H
#define APP_TOOLS_AUTO_SHADE_PALETTE_GENERATOR_H

#include "shade_types.h"  // For PaletteMaterial, PaletteStyle

#include "doc/color.h"
#include "gfx/point.h"

#include <vector>

namespace app {
namespace tools {

// PaletteMaterial and PaletteStyle enums are defined in shade_types.h

// HSV color representation
struct HSV {
    double h;  // Hue: 0-360
    double s;  // Saturation: 0-1
    double v;  // Value: 0-1

    HSV() : h(0), s(0), v(0) {}
    HSV(double hue, double sat, double val) : h(hue), s(sat), v(val) {}
};

// Extended palette generation settings
struct PaletteSettings {
    int colorCount;             // 3, 5, 7, or 9 colors
    double hueShift;            // 0-60 degrees of hue shift across ramp
    double saturationPeak;      // 0-1, where saturation peaks (0.5 = midtones)
    double saturationRange;     // 0-1, how much saturation varies
    double valueRange;          // 0-1, range from darkest to lightest
    ColorHarmony harmony;       // Color harmony mode
    TemperatureShift temperature; // Warm/cool shift direction

    PaletteSettings()
        : colorCount(5)
        , hueShift(20.0)
        , saturationPeak(0.5)
        , saturationRange(0.3)
        , valueRange(0.7)
        , harmony(ColorHarmony::Monochromatic)
        , temperature(TemperatureShift::WarmHighlights)
    {}
};

// Generated palette result
struct GeneratedPalette {
    doc::color_t deepShadow;    // Darkest shadow
    doc::color_t shadow;        // Main shadow
    doc::color_t midShadow;     // Shadow-to-base transition
    doc::color_t base;          // Base color (adjusted from input)
    doc::color_t midHighlight;  // Base-to-highlight transition
    doc::color_t highlight;     // Main highlight
    doc::color_t specular;      // Brightest specular (optional)

    // Extended colors for 7-9 color ramps
    std::vector<doc::color_t> colors;  // All colors in order (dark to light)

    // Get as vector for easy iteration
    std::vector<doc::color_t> asVector() const {
        return { deepShadow, shadow, midShadow, base, midHighlight, highlight, specular };
    }

    // Get core 5 colors (excluding deep shadow and specular)
    std::vector<doc::color_t> coreColors() const {
        return { shadow, midShadow, base, midHighlight, highlight };
    }

    // Get all colors based on the generated count
    const std::vector<doc::color_t>& allColors() const {
        return colors;
    }
};

class PaletteGenerator {
public:
    // Generate a complete shading palette from base color
    static GeneratedPalette generate(
        doc::color_t baseColor,
        double lightAngle,          // 0-360 degrees, 0=right, 90=down
        PaletteMaterial material,
        PaletteStyle style);

    // Generate with full settings control
    static GeneratedPalette generateWithSettings(
        doc::color_t baseColor,
        double lightAngle,
        PaletteMaterial material,
        PaletteStyle style,
        const PaletteSettings& settings);

    // Generate with custom parameters
    static GeneratedPalette generateCustom(
        doc::color_t baseColor,
        double lightAngle,
        double hueShiftStrength,    // 0-1, how much to shift hue
        double saturationBoost,     // -1 to 1, modify saturation
        double contrastMultiplier,  // 0.5-2, affect value spread
        bool warmLight);            // true=warm light/cool shadows

    // Generate harmony colors from base
    static std::vector<doc::color_t> generateHarmonyColors(
        doc::color_t baseColor,
        ColorHarmony harmony);

    // Color conversion utilities
    static HSV rgbToHsv(int r, int g, int b);
    static void hsvToRgb(const HSV& hsv, int& r, int& g, int& b);

    // Extract RGB from doc::color_t
    static void extractRgb(doc::color_t color, int& r, int& g, int& b);
    static doc::color_t makeColor(int r, int g, int b, int a = 255);

private:
    // Get material-specific parameters
    struct MaterialParams {
        double shadowValueMult;     // How dark shadows get (0-1)
        double highlightValueMult;  // How bright highlights get (1-2)
        double shadowSatMult;       // Shadow saturation modifier
        double highlightSatMult;    // Highlight saturation modifier
        double contrastBoost;       // Overall contrast adjustment
    };
    static MaterialParams getMaterialParams(PaletteMaterial material);

    // Get style-specific parameters
    struct StyleParams {
        double hueShiftMax;         // Maximum hue shift in degrees
        double saturationCurve;     // How saturation peaks in midtones
        double valueSpread;         // How spread out values are
    };
    static StyleParams getStyleParams(PaletteStyle style);

    // Calculate hue shift based on light angle and position in value ramp
    static double calculateHueShift(
        double lightAngle,
        double valuePosition,       // -1 (shadow) to +1 (highlight)
        double maxShift,
        bool warmLight);

    // Apply saturation curve (peaks in midtones)
    static double applySaturationCurve(
        double baseSat,
        double valuePosition,
        double curveFactor);

    // Clamp and wrap utilities
    static double clampValue(double v, double min, double max);
    static double wrapHue(double h);
};

} // namespace tools
} // namespace app

#endif // APP_TOOLS_AUTO_SHADE_PALETTE_GENERATOR_H
