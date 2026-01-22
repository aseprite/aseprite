// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Shading Calculator
//
// Handles light intensity calculation and color mapping.

#ifndef APP_TOOLS_AUTO_SHADE_SHADING_CALCULATOR_H
#define APP_TOOLS_AUTO_SHADE_SHADING_CALCULATOR_H

#include "shade_types.h"
#include "doc/color.h"

namespace app {
namespace tools {

class ShadingCalculator {
public:
    // Calculate light intensity for a pixel based on its normal
    static double calculateIntensity(
        const Vector2D& normal,
        const Vector2D& lightDirection,
        double ambientLevel = 0.2);

    // Map intensity to color based on shading mode
    static doc::color_t intensityToColor(
        double intensity,
        const ShadeConfig& config);

    // Interpolate between two colors
    static doc::color_t interpolateColor(
        doc::color_t c1,
        doc::color_t c2,
        double t);

    // Add specular highlight if applicable
    static doc::color_t addSpecular(
        doc::color_t baseColor,
        const Vector2D& normal,
        const Vector2D& lightDirection,
        const ShadeConfig& config);

    // Add rim light if applicable
    static doc::color_t addRimLight(
        doc::color_t baseColor,
        const Vector2D& normal,
        const Vector2D& lightDirection,
        bool isEdgePixel,
        const ShadeConfig& config);

    // Calculate full shading for a pixel
    static doc::color_t shade(
        const Vector2D& normal,
        const Vector2D& lightDirection,
        bool isEdgePixel,
        const ShadeConfig& config);

    // Calculate shading with reflected light support
    // distanceToEdge: 0 = on edge, higher = more interior
    // maxDistance: maximum distance from edge in the shape
    static doc::color_t shadeWithReflectedLight(
        const Vector2D& normal,
        const Vector2D& lightDirection,
        float distanceToEdge,
        float maxDistance,
        bool isEdgePixel,
        const ShadeConfig& config);

private:
    // Map intensity to 3-shade color
    static doc::color_t intensity3Shade(
        double intensity,
        doc::color_t shadow,
        doc::color_t base,
        doc::color_t highlight);

    // Map intensity to 5-shade color
    static doc::color_t intensity5Shade(
        double intensity,
        doc::color_t shadow,
        doc::color_t midShadow,
        doc::color_t base,
        doc::color_t midHighlight,
        doc::color_t highlight);

    // Map intensity to gradient color
    static doc::color_t intensityGradient(
        double intensity,
        doc::color_t shadow,
        doc::color_t base,
        doc::color_t highlight);
};

} // namespace tools
} // namespace app

#endif // APP_TOOLS_AUTO_SHADE_SHADING_CALCULATOR_H
