// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Palette Generator Implementation

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "palette_generator.h"

#define _USE_MATH_DEFINES
#include <algorithm>
#include <cmath>
#include <cstdio>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PALETTE_LOG(fmt, ...) printf("[PaletteGen] " fmt "\n", ##__VA_ARGS__)

namespace app {
namespace tools {

// ============================================================================
// Color Conversion
// ============================================================================

void PaletteGenerator::extractRgb(doc::color_t color, int& r, int& g, int& b)
{
    r = doc::rgba_getr(color);
    g = doc::rgba_getg(color);
    b = doc::rgba_getb(color);
}

doc::color_t PaletteGenerator::makeColor(int r, int g, int b, int a)
{
    r = std::clamp(r, 0, 255);
    g = std::clamp(g, 0, 255);
    b = std::clamp(b, 0, 255);
    a = std::clamp(a, 0, 255);
    return doc::rgba(r, g, b, a);
}

HSV PaletteGenerator::rgbToHsv(int r, int g, int b)
{
    double rd = r / 255.0;
    double gd = g / 255.0;
    double bd = b / 255.0;

    double maxVal = std::max({rd, gd, bd});
    double minVal = std::min({rd, gd, bd});
    double delta = maxVal - minVal;

    HSV hsv;
    hsv.v = maxVal;

    if (delta < 0.00001) {
        hsv.s = 0;
        hsv.h = 0;
        return hsv;
    }

    hsv.s = (maxVal > 0) ? (delta / maxVal) : 0;

    if (rd >= maxVal) {
        hsv.h = 60.0 * fmod((gd - bd) / delta, 6.0);
    }
    else if (gd >= maxVal) {
        hsv.h = 60.0 * ((bd - rd) / delta + 2.0);
    }
    else {
        hsv.h = 60.0 * ((rd - gd) / delta + 4.0);
    }

    if (hsv.h < 0) {
        hsv.h += 360.0;
    }

    return hsv;
}

void PaletteGenerator::hsvToRgb(const HSV& hsv, int& r, int& g, int& b)
{
    double h = hsv.h;
    double s = clampValue(hsv.s, 0, 1);
    double v = clampValue(hsv.v, 0, 1);

    if (s < 0.00001) {
        r = g = b = static_cast<int>(v * 255);
        return;
    }

    h = fmod(h, 360.0);
    if (h < 0) h += 360.0;
    h /= 60.0;

    int i = static_cast<int>(h);
    double f = h - i;
    double p = v * (1.0 - s);
    double q = v * (1.0 - s * f);
    double t = v * (1.0 - s * (1.0 - f));

    double rd, gd, bd;
    switch (i) {
        case 0:  rd = v; gd = t; bd = p; break;
        case 1:  rd = q; gd = v; bd = p; break;
        case 2:  rd = p; gd = v; bd = t; break;
        case 3:  rd = p; gd = q; bd = v; break;
        case 4:  rd = t; gd = p; bd = v; break;
        default: rd = v; gd = p; bd = q; break;
    }

    r = static_cast<int>(rd * 255);
    g = static_cast<int>(gd * 255);
    b = static_cast<int>(bd * 255);
}

// ============================================================================
// Utility Functions
// ============================================================================

double PaletteGenerator::clampValue(double v, double min, double max)
{
    return std::max(min, std::min(max, v));
}

double PaletteGenerator::wrapHue(double h)
{
    h = fmod(h, 360.0);
    if (h < 0) h += 360.0;
    return h;
}

// ============================================================================
// Material Parameters
// ============================================================================

PaletteGenerator::MaterialParams PaletteGenerator::getMaterialParams(PaletteMaterial material)
{
    MaterialParams params;

    switch (material) {
        case PaletteMaterial::Matte:
            params.shadowValueMult = 0.45;     // Moderate shadows
            params.highlightValueMult = 1.25;  // Subtle highlights
            params.shadowSatMult = 0.85;       // Slight desaturation
            params.highlightSatMult = 0.70;    // More desaturation in highlights
            params.contrastBoost = 1.0;
            break;

        case PaletteMaterial::Glossy:
            params.shadowValueMult = 0.35;     // Darker shadows
            params.highlightValueMult = 1.50;  // Brighter highlights
            params.shadowSatMult = 0.90;       // Keep saturation
            params.highlightSatMult = 0.50;    // Desaturate highlights
            params.contrastBoost = 1.2;
            break;

        case PaletteMaterial::Metallic:
            params.shadowValueMult = 0.25;     // Very dark shadows
            params.highlightValueMult = 1.80;  // Very bright highlights
            params.shadowSatMult = 0.40;       // Desaturate shadows heavily
            params.highlightSatMult = 0.30;    // Desaturate highlights heavily
            params.contrastBoost = 1.5;
            break;

        case PaletteMaterial::Skin:
            params.shadowValueMult = 0.50;     // Not too dark (subsurface)
            params.highlightValueMult = 1.20;  // Subtle highlights
            params.shadowSatMult = 1.10;       // Slightly more saturated shadows
            params.highlightSatMult = 0.60;    // Desaturated highlights
            params.contrastBoost = 0.9;
            break;
    }

    return params;
}

// ============================================================================
// Style Parameters
// ============================================================================

PaletteGenerator::StyleParams PaletteGenerator::getStyleParams(PaletteStyle style)
{
    StyleParams params;

    switch (style) {
        case PaletteStyle::Natural:
            params.hueShiftMax = 18.0;     // Subtle shift
            params.saturationCurve = 0.8;  // Moderate midtone boost
            params.valueSpread = 1.0;
            break;

        case PaletteStyle::Vibrant:
            params.hueShiftMax = 28.0;     // Strong shift
            params.saturationCurve = 1.2;  // High midtone saturation
            params.valueSpread = 1.1;
            break;

        case PaletteStyle::Muted:
            params.hueShiftMax = 12.0;     // Minimal shift
            params.saturationCurve = 0.5;  // Low saturation overall
            params.valueSpread = 0.85;
            break;

        case PaletteStyle::Dramatic:
            params.hueShiftMax = 22.0;     // Moderate-strong shift
            params.saturationCurve = 0.9;  // Good saturation
            params.valueSpread = 1.3;      // Wide value range
            break;
    }

    return params;
}

// ============================================================================
// Hue Shift Calculation
// ============================================================================

double PaletteGenerator::calculateHueShift(
    double lightAngle,
    double valuePosition,
    double maxShift,
    bool warmLight)
{
    // valuePosition: -1 = deep shadow, 0 = midtone, +1 = highlight
    //
    // Color theory for warm light source:
    // - Shadows become COOL (shift toward blue/purple = DECREASE hue toward magenta)
    // - Highlights become WARM (shift toward yellow/orange = INCREASE hue)
    //
    // On the hue wheel (0=red, 60=yellow, 120=green, 180=cyan, 240=blue, 300=magenta):
    // - To make red cooler: shift toward magenta (-30°) or purple
    // - To make red warmer: shift toward orange (+30°)
    //
    // Light angle affects intensity:
    // - Top light (270°) = cooler sky light
    // - Bottom light (90°) = warmer ground reflection

    // Light angle influence on shift intensity
    double angleRad = lightAngle * M_PI / 180.0;
    double verticalInfluence = sin(angleRad) * 0.3;
    double horizontalInfluence = cos(angleRad) * 0.15;
    double angleInfluence = 1.0 + verticalInfluence + horizontalInfluence;

    // Calculate shift amount based on distance from midtone
    double shiftAmount = std::abs(valuePosition) * maxShift * angleInfluence;

    // Apply direction based on warm/cool light and position
    if (warmLight) {
        // Warm light: cool shadows, warm highlights
        if (valuePosition < 0) {
            // Shadow: shift toward cool (magenta/purple) = NEGATIVE hue
            return -shiftAmount;
        }
        else {
            // Highlight: shift toward warm (orange/yellow) = POSITIVE hue
            return shiftAmount;
        }
    }
    else {
        // Cool light: warm shadows, cool highlights (opposite)
        if (valuePosition < 0) {
            // Shadow: shift toward warm = POSITIVE hue
            return shiftAmount;
        }
        else {
            // Highlight: shift toward cool = NEGATIVE hue
            return -shiftAmount;
        }
    }
}

// ============================================================================
// Saturation Curve
// ============================================================================

double PaletteGenerator::applySaturationCurve(
    double baseSat,
    double valuePosition,
    double curveFactor)
{
    // Saturation should peak in midtones and decrease at extremes
    // This creates more natural-looking palettes
    //
    // The curve: sat = baseSat * (1 - abs(valuePosition)^power)
    // where power < 1 creates a gentle curve, power > 1 creates sharper falloff

    double falloff = std::pow(std::abs(valuePosition), 1.5);
    double satMod = 1.0 - falloff * (1.0 - curveFactor * 0.5);

    return clampValue(baseSat * satMod, 0, 1);
}

// ============================================================================
// Main Generation Functions
// ============================================================================

GeneratedPalette PaletteGenerator::generate(
    doc::color_t baseColor,
    double lightAngle,
    PaletteMaterial material,
    PaletteStyle style)
{
    PaletteSettings settings;
    return generateWithSettings(baseColor, lightAngle, material, style, settings);
}

GeneratedPalette PaletteGenerator::generateWithSettings(
    doc::color_t baseColor,
    double lightAngle,
    PaletteMaterial material,
    PaletteStyle style,
    const PaletteSettings& settings)
{
    MaterialParams matParams = getMaterialParams(material);
    StyleParams styleParams = getStyleParams(style);

    // Determine temperature shift direction
    // WarmHighlights: shadows go cool (blue), highlights go warm (yellow)
    // CoolHighlights: shadows go warm (yellow), highlights go cool (blue)
    bool warmLight = true;
    if (settings.temperature == TemperatureShift::CoolHighlights) {
        warmLight = false;
    }
    else if (settings.temperature == TemperatureShift::WarmHighlights) {
        // Use light angle to influence warmth direction
        // Top-left light (135°) = typical warm highlights
        // Bottom light (270°) = cooler (reflected light)
        warmLight = true;
    }
    // Neutral = no temperature-based shift, but hue shift still applies

    // Determine the maximum hue shift amount
    // Use settings hue shift if provided, otherwise use style default
    double maxHueShift = settings.hueShift;
    if (maxHueShift <= 0) {
        maxHueShift = styleParams.hueShiftMax;
    }

    // Generate base palette
    int r, g, b;
    extractRgb(baseColor, r, g, b);
    int alpha = doc::rgba_geta(baseColor);
    HSV baseHsv = rgbToHsv(r, g, b);

    // Calculate harmony-based hue offsets
    // These affect where shadows and highlights shift on the color wheel
    double shadowHueOffset = 0;    // Additional hue offset for shadows
    double highlightHueOffset = 0; // Additional hue offset for highlights

    switch (settings.harmony) {
        case ColorHarmony::Monochromatic:
            // No additional hue shifts - just temperature-based
            break;

        case ColorHarmony::Analogous:
            // Shadows shift to one adjacent hue, highlights to other
            shadowHueOffset = -30.0;   // Shift shadows toward adjacent hue
            highlightHueOffset = 30.0; // Shift highlights toward other adjacent hue
            break;

        case ColorHarmony::Complementary:
            // Shadows shift toward complementary color
            shadowHueOffset = 60.0;    // Move shadows toward complement (halfway)
            highlightHueOffset = -15.0; // Slight shift for highlights
            break;

        case ColorHarmony::SplitComplementary:
            // Use split-complementary colors for shadow/highlight
            shadowHueOffset = 150.0;   // One side of complement
            highlightHueOffset = -30.0; // Slight opposite shift
            break;

        case ColorHarmony::Triadic:
            // Distribute triadic colors: shadows one way, highlights another
            shadowHueOffset = 120.0;   // First triadic color for shadows
            highlightHueOffset = -40.0; // Hint toward other triadic
            break;

        case ColorHarmony::Tetradic:
            // More complex: shadows toward 90°, highlights toward 270°
            shadowHueOffset = 90.0;
            highlightHueOffset = -45.0;
            break;
    }

    // Generate colors based on color count
    int colorCount = settings.colorCount;
    if (colorCount < 3) colorCount = 3;
    if (colorCount > 9) colorCount = 9;

    GeneratedPalette palette;
    palette.colors.resize(colorCount);

    // Calculate positions for each color
    for (int i = 0; i < colorCount; ++i) {
        // Position from -1 (darkest) to +1 (lightest)
        double pos = (colorCount == 1) ? 0.0 :
            -1.0 + 2.0 * static_cast<double>(i) / static_cast<double>(colorCount - 1);

        // Value calculation with contrast
        double valueBase = 0.5 + pos * 0.5 * settings.valueRange;
        double newValue = clampValue(baseHsv.v * (0.4 + valueBase * 1.2), 0, 1);

        // Apply material contrast
        newValue = clampValue(newValue * matParams.contrastBoost, 0, 1);

        // Hue shift calculation - always apply if hue shift > 0
        double hueShiftAmount = 0;
        if (maxHueShift > 0) {
            if (settings.temperature == TemperatureShift::Neutral) {
                // Neutral: simple linear hue shift based on position
                double angleInfluence = cos(lightAngle * M_PI / 180.0);
                hueShiftAmount = pos * maxHueShift * angleInfluence;
            }
            else {
                // Temperature-based shift with light angle influence
                hueShiftAmount = calculateHueShift(lightAngle, pos, maxHueShift, warmLight);
            }
        }

        // Apply harmony-based hue offset
        // Blend from shadow offset (pos=-1) through base (pos=0) to highlight offset (pos=1)
        double harmonyOffset = 0;
        if (pos < 0) {
            // Shadow region: blend from shadow offset to 0
            harmonyOffset = shadowHueOffset * (-pos);  // -pos gives 0-1 range
        }
        else {
            // Highlight region: blend from 0 to highlight offset
            harmonyOffset = highlightHueOffset * pos;
        }

        double newHue = wrapHue(baseHsv.h + hueShiftAmount + harmonyOffset);

        // Saturation curve - peaks at saturationPeak position
        double distFromPeak = std::abs(pos - (settings.saturationPeak * 2.0 - 1.0));
        double satMod = 1.0 - distFromPeak * settings.saturationRange;
        double newSat = clampValue(baseHsv.s * satMod * styleParams.saturationCurve, 0, 1);

        // Material-specific saturation adjustments
        if (pos < -0.3) {
            newSat *= matParams.shadowSatMult;
        }
        else if (pos > 0.3) {
            newSat *= matParams.highlightSatMult;
        }

        HSV colorHsv(newHue, newSat, newValue);
        int outR, outG, outB;
        hsvToRgb(colorHsv, outR, outG, outB);
        palette.colors[i] = makeColor(outR, outG, outB, alpha);
    }

    // Map to legacy fields for backwards compatibility
    if (colorCount >= 3) {
        palette.shadow = palette.colors[0];
        palette.base = palette.colors[colorCount / 2];
        palette.highlight = palette.colors[colorCount - 1];
    }
    if (colorCount >= 5) {
        palette.midShadow = palette.colors[1];
        palette.midHighlight = palette.colors[colorCount - 2];
    }
    if (colorCount >= 7) {
        palette.deepShadow = palette.colors[0];
        palette.shadow = palette.colors[1];
        palette.specular = palette.colors[colorCount - 1];
        palette.highlight = palette.colors[colorCount - 2];
    }

    return palette;
}

std::vector<doc::color_t> PaletteGenerator::generateHarmonyColors(
    doc::color_t baseColor,
    ColorHarmony harmony)
{
    std::vector<doc::color_t> colors;
    int r, g, b;
    extractRgb(baseColor, r, g, b);
    int alpha = doc::rgba_geta(baseColor);
    HSV baseHsv = rgbToHsv(r, g, b);

    colors.push_back(baseColor);

    switch (harmony) {
        case ColorHarmony::Monochromatic:
            // Just the base color variations (handled by ramp)
            break;

        case ColorHarmony::Analogous:
            {
                // ±30° from base
                HSV h1(wrapHue(baseHsv.h - 30), baseHsv.s, baseHsv.v);
                HSV h2(wrapHue(baseHsv.h + 30), baseHsv.s, baseHsv.v);
                int r1, g1, b1, r2, g2, b2;
                hsvToRgb(h1, r1, g1, b1);
                hsvToRgb(h2, r2, g2, b2);
                colors.push_back(makeColor(r1, g1, b1, alpha));
                colors.push_back(makeColor(r2, g2, b2, alpha));
            }
            break;

        case ColorHarmony::Complementary:
            {
                // 180° from base
                HSV h1(wrapHue(baseHsv.h + 180), baseHsv.s, baseHsv.v);
                int r1, g1, b1;
                hsvToRgb(h1, r1, g1, b1);
                colors.push_back(makeColor(r1, g1, b1, alpha));
            }
            break;

        case ColorHarmony::SplitComplementary:
            {
                // 150° and 210° from base
                HSV h1(wrapHue(baseHsv.h + 150), baseHsv.s, baseHsv.v);
                HSV h2(wrapHue(baseHsv.h + 210), baseHsv.s, baseHsv.v);
                int r1, g1, b1, r2, g2, b2;
                hsvToRgb(h1, r1, g1, b1);
                hsvToRgb(h2, r2, g2, b2);
                colors.push_back(makeColor(r1, g1, b1, alpha));
                colors.push_back(makeColor(r2, g2, b2, alpha));
            }
            break;

        case ColorHarmony::Triadic:
            {
                // 120° and 240° from base
                HSV h1(wrapHue(baseHsv.h + 120), baseHsv.s, baseHsv.v);
                HSV h2(wrapHue(baseHsv.h + 240), baseHsv.s, baseHsv.v);
                int r1, g1, b1, r2, g2, b2;
                hsvToRgb(h1, r1, g1, b1);
                hsvToRgb(h2, r2, g2, b2);
                colors.push_back(makeColor(r1, g1, b1, alpha));
                colors.push_back(makeColor(r2, g2, b2, alpha));
            }
            break;

        case ColorHarmony::Tetradic:
            {
                // 90°, 180°, 270° from base
                HSV h1(wrapHue(baseHsv.h + 90), baseHsv.s, baseHsv.v);
                HSV h2(wrapHue(baseHsv.h + 180), baseHsv.s, baseHsv.v);
                HSV h3(wrapHue(baseHsv.h + 270), baseHsv.s, baseHsv.v);
                int r1, g1, b1, r2, g2, b2, r3, g3, b3;
                hsvToRgb(h1, r1, g1, b1);
                hsvToRgb(h2, r2, g2, b2);
                hsvToRgb(h3, r3, g3, b3);
                colors.push_back(makeColor(r1, g1, b1, alpha));
                colors.push_back(makeColor(r2, g2, b2, alpha));
                colors.push_back(makeColor(r3, g3, b3, alpha));
            }
            break;
    }

    return colors;
}

GeneratedPalette PaletteGenerator::generateCustom(
    doc::color_t baseColor,
    double lightAngle,
    double hueShiftStrength,
    double saturationBoost,
    double contrastMultiplier,
    bool warmLight)
{
    int r, g, b;
    extractRgb(baseColor, r, g, b);
    int alpha = doc::rgba_geta(baseColor);

    HSV baseHsv = rgbToHsv(r, g, b);

    PALETTE_LOG("Generating palette: base HSV=(%.1f, %.2f, %.2f), light=%.0f°, warm=%d",
                baseHsv.h, baseHsv.s, baseHsv.v, lightAngle, warmLight);

    // Define value positions for each palette slot
    // These are relative positions from shadow (-1) to highlight (+1)
    const double positions[] = {
        -1.0,   // Deep shadow
        -0.7,   // Shadow
        -0.35,  // Mid-shadow
         0.0,   // Base
         0.35,  // Mid-highlight
         0.7,   // Highlight
         1.0    // Specular
    };

    // Value multipliers at each position
    const double valueMultipliers[] = {
        0.25,   // Deep shadow
        0.45,   // Shadow
        0.70,   // Mid-shadow
        1.0,    // Base
        1.15,   // Mid-highlight
        1.35,   // Highlight
        1.60    // Specular
    };

    // Generate colors
    GeneratedPalette palette;
    doc::color_t* colors[] = {
        &palette.deepShadow,
        &palette.shadow,
        &palette.midShadow,
        &palette.base,
        &palette.midHighlight,
        &palette.highlight,
        &palette.specular
    };

    double maxHueShift = 25.0 * hueShiftStrength;  // Max degrees of shift

    for (int i = 0; i < 7; ++i) {
        double pos = positions[i];
        double valueMult = valueMultipliers[i];

        // Apply contrast multiplier to value
        double valueOffset = (valueMult - 1.0) * contrastMultiplier;
        double newValue = clampValue(baseHsv.v * (1.0 + valueOffset), 0, 1);

        // Calculate hue shift
        double hueShift = calculateHueShift(lightAngle, pos, maxHueShift, warmLight);
        double newHue = wrapHue(baseHsv.h + hueShift);

        // Apply saturation curve
        double baseSatWithBoost = clampValue(baseHsv.s * (1.0 + saturationBoost), 0, 1);
        double newSat = applySaturationCurve(baseSatWithBoost, pos, 0.8);

        // Additional saturation adjustments for extremes
        if (pos < -0.5) {
            // Shadows: can desaturate based on material
            newSat *= 0.85;
        }
        else if (pos > 0.5) {
            // Highlights: typically desaturate
            newSat *= 0.7 - (pos - 0.5) * 0.4;
        }

        HSV colorHsv(newHue, newSat, newValue);
        int outR, outG, outB;
        hsvToRgb(colorHsv, outR, outG, outB);

        *colors[i] = makeColor(outR, outG, outB, alpha);

        PALETTE_LOG("  [%d] pos=%.2f: HSV(%.1f, %.2f, %.2f) -> RGB(%d, %d, %d)",
                    i, pos, newHue, newSat, newValue, outR, outG, outB);
    }

    return palette;
}

} // namespace tools
} // namespace app
