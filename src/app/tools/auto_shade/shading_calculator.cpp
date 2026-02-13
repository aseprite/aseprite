// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Shading Calculator Implementation

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "shading_calculator.h"
#include "doc/image.h"

#include <cmath>
#include <algorithm>

namespace app {
namespace tools {

double ShadingCalculator::calculateIntensity(
    const Vector2D& normal,
    const Vector2D& lightDirection,
    double ambientLevel)
{
    // Dot product: measures alignment between normal and light
    // -1 = facing directly away
    //  0 = perpendicular
    // +1 = facing directly toward
    double dotProduct = normal.dot(lightDirection);

    // Only positive values contribute to diffuse lighting
    // (surfaces facing away from light receive no direct light)
    double diffuse = std::max(0.0, dotProduct);

    // Combine ambient and diffuse
    // Ambient ensures even shadows aren't completely black
    double intensity = ambientLevel + (1.0 - ambientLevel) * diffuse;

    // Clamp to [0, 1]
    return clamp(intensity, 0.0, 1.0);
}

doc::color_t ShadingCalculator::intensityToColor(
    double intensity,
    const ShadeConfig& config)
{
    switch (config.shadingMode) {
        case ShadingMode::ThreeShade:
            return intensity3Shade(
                intensity,
                config.shadowColor,
                config.baseColor,
                config.highlightColor);

        case ShadingMode::FiveShade:
            return intensity5Shade(
                intensity,
                config.shadowColor,
                config.midShadowColor,
                config.baseColor,
                config.midHighlightColor,
                config.highlightColor);

        case ShadingMode::Gradient:
        default:
            return intensityGradient(
                intensity,
                config.shadowColor,
                config.baseColor,
                config.highlightColor);
    }
}

doc::color_t ShadingCalculator::interpolateColor(
    doc::color_t c1,
    doc::color_t c2,
    double t)
{
    // Clamp t to [0, 1]
    t = clamp(t, 0.0, 1.0);

    int r1 = doc::rgba_getr(c1);
    int g1 = doc::rgba_getg(c1);
    int b1 = doc::rgba_getb(c1);
    int a1 = doc::rgba_geta(c1);

    int r2 = doc::rgba_getr(c2);
    int g2 = doc::rgba_getg(c2);
    int b2 = doc::rgba_getb(c2);
    int a2 = doc::rgba_geta(c2);

    int r = static_cast<int>(lerp(r1, r2, t));
    int g = static_cast<int>(lerp(g1, g2, t));
    int b = static_cast<int>(lerp(b1, b2, t));
    int a = static_cast<int>(lerp(a1, a2, t));

    // Clamp to valid range
    r = clampInt(r, 0, 255);
    g = clampInt(g, 0, 255);
    b = clampInt(b, 0, 255);
    a = clampInt(a, 0, 255);

    return doc::rgba(r, g, b, a);
}

doc::color_t ShadingCalculator::addSpecular(
    doc::color_t baseColor,
    const Vector2D& normal,
    const Vector2D& lightDirection,
    const ShadeConfig& config)
{
    if (!config.enableSpecular) {
        return baseColor;
    }

    // View direction: assume orthographic view looking straight at screen
    Vector2D viewDir(0, -1);

    // Calculate reflection of light direction around normal
    // R = 2(N·L)N - L
    double dotNL = normal.dot(lightDirection);
    Vector2D reflectDir(
        2.0 * dotNL * normal.x - lightDirection.x,
        2.0 * dotNL * normal.y - lightDirection.y
    );

    // Specular intensity based on view-reflection alignment
    double dotRV = reflectDir.dot(viewDir);
    dotRV = std::max(0.0, dotRV);

    // Apply shininess (higher = smaller, tighter highlight)
    double specIntensity = std::pow(dotRV, config.specularShininess);

    // Only apply if above threshold
    if (specIntensity > config.specularThreshold) {
        // Blend with specular color based on intensity
        double blendFactor = (specIntensity - config.specularThreshold) /
                            (1.0 - config.specularThreshold);
        return interpolateColor(baseColor, config.specularColor, blendFactor * 0.8);
    }

    return baseColor;
}

doc::color_t ShadingCalculator::addRimLight(
    doc::color_t baseColor,
    const Vector2D& normal,
    const Vector2D& lightDirection,
    bool isEdgePixel,
    const ShadeConfig& config)
{
    if (!config.enableRimLight || !isEdgePixel) {
        return baseColor;
    }

    // Rim light appears on edges facing away from light
    double dotNL = normal.dot(lightDirection);

    // If facing away from light (negative dot product)
    if (dotNL < -0.3) {
        // Calculate rim light intensity
        double rimFactor = std::abs(dotNL + 0.3) / 0.7;  // Remap to [0, 1]
        rimFactor = clamp(rimFactor * config.rimLightIntensity, 0.0, 0.5);

        // Blend with highlight color for rim effect
        return interpolateColor(baseColor, config.highlightColor, rimFactor);
    }

    return baseColor;
}

doc::color_t ShadingCalculator::shade(
    const Vector2D& normal,
    const Vector2D& lightDirection,
    bool isEdgePixel,
    const ShadeConfig& config)
{
    // Calculate base intensity
    double intensity = calculateIntensity(normal, lightDirection, config.ambientLevel);

    // Map to color
    doc::color_t color = intensityToColor(intensity, config);

    // Add specular highlights (for glossy materials)
    if (config.materialType == MaterialType::Glossy) {
        color = addSpecular(color, normal, lightDirection, config);
    }

    // Add rim light
    color = addRimLight(color, normal, lightDirection, isEdgePixel, config);

    return color;
}

doc::color_t ShadingCalculator::intensity3Shade(
    double intensity,
    doc::color_t shadow,
    doc::color_t base,
    doc::color_t highlight)
{
    // Split intensity into 3 regions
    if (intensity < 0.33) {
        return shadow;
    }
    else if (intensity < 0.66) {
        return base;
    }
    else {
        return highlight;
    }
}

doc::color_t ShadingCalculator::intensity5Shade(
    double intensity,
    doc::color_t shadow,
    doc::color_t midShadow,
    doc::color_t base,
    doc::color_t midHighlight,
    doc::color_t highlight)
{
    // Split intensity into 5 regions
    if (intensity < 0.2) {
        return shadow;
    }
    else if (intensity < 0.4) {
        return midShadow;
    }
    else if (intensity < 0.6) {
        return base;
    }
    else if (intensity < 0.8) {
        return midHighlight;
    }
    else {
        return highlight;
    }
}

doc::color_t ShadingCalculator::intensityGradient(
    double intensity,
    doc::color_t shadow,
    doc::color_t base,
    doc::color_t highlight)
{
    if (intensity < 0.5) {
        // Interpolate between shadow and base
        double t = intensity * 2.0;  // Map [0, 0.5] to [0, 1]
        return interpolateColor(shadow, base, t);
    }
    else {
        // Interpolate between base and highlight
        double t = (intensity - 0.5) * 2.0;  // Map [0.5, 1] to [0, 1]
        return interpolateColor(base, highlight, t);
    }
}

doc::color_t ShadingCalculator::shadeWithReflectedLight(
    const Vector2D& normal,
    const Vector2D& lightDirection,
    float distanceToEdge,
    float maxDistance,
    bool isEdgePixel,
    const ShadeConfig& config)
{
    // Calculate base intensity from normal and light
    double intensity = calculateIntensity(normal, lightDirection, config.ambientLevel);

    // Check if this pixel is in shadow (intensity below threshold)
    // Shadow threshold: below 0.4 is considered shadow area
    const double shadowThreshold = 0.4;
    bool isInShadow = intensity < shadowThreshold;

    // Apply reflected light logic for shadow-side edge pixels
    // From the guide: "Never let the shadow extend all the way to the edge.
    // Always leave a rim of reflected light to emphasize the round form."
    if (config.enableReflectedLight && isInShadow) {
        int reflectedWidth = config.reflectedLightWidth;

        // Scale reflected light width based on shape size
        // For very small shapes, use at least 1 pixel
        // For larger shapes, the configured width
        if (maxDistance > 0 && maxDistance < 5) {
            reflectedWidth = 1;  // Small shapes: 1 pixel reflected light
        }

        // If pixel is near the edge (within reflected light threshold)
        if (distanceToEdge < reflectedWidth) {
            // Use base color for reflected light
            // This creates the classic pixel art sphere look
            doc::color_t color = config.baseColor;

            // Optional: slightly modulate based on how close to edge
            // Pixels right on the edge can be slightly brighter
            if (distanceToEdge < 1 && config.enableRimLight) {
                // Very edge pixels get a tiny boost
                color = interpolateColor(config.baseColor, config.highlightColor, 0.1);
            }

            return color;
        }
    }

    // For non-shadow areas or interior shadow pixels, use normal shading
    doc::color_t color = intensityToColor(intensity, config);

    // Add specular highlights (for glossy materials)
    if (config.materialType == MaterialType::Glossy) {
        color = addSpecular(color, normal, lightDirection, config);
    }

    // Add rim light (different from reflected light - this is for lit edges)
    color = addRimLight(color, normal, lightDirection, isEdgePixel, config);

    return color;
}

} // namespace tools
} // namespace app
