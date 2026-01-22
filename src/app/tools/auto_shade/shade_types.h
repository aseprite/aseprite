// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Common types and structures

#ifndef APP_TOOLS_AUTO_SHADE_SHADE_TYPES_H
#define APP_TOOLS_AUTO_SHADE_SHADE_TYPES_H

#include "gfx/point.h"
#include "gfx/rect.h"
#include "doc/color.h"

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>

namespace app {
namespace tools {

//------------------------------------------------------------------------------
// Point hash for use in unordered containers
//------------------------------------------------------------------------------

struct PointHash {
    std::size_t operator()(const gfx::Point& p) const {
        return std::hash<int>()(p.x) ^ (std::hash<int>()(p.y) << 16);
    }
};

using PointSet = std::unordered_set<gfx::Point, PointHash>;
using DistanceMap = std::unordered_map<gfx::Point, float, PointHash>;

//------------------------------------------------------------------------------
// 2D Vector for normals and light direction
//------------------------------------------------------------------------------

struct Vector2D {
    double x;
    double y;

    Vector2D() : x(0), y(0) {}
    Vector2D(double x_, double y_) : x(x_), y(y_) {}

    double length() const {
        return std::sqrt(x * x + y * y);
    }

    Vector2D normalized() const {
        double len = length();
        if (len == 0) return Vector2D(0, -1);  // Default: up
        return Vector2D(x / len, y / len);
    }

    double dot(const Vector2D& other) const {
        return x * other.x + y * other.y;
    }

    Vector2D operator+(const Vector2D& other) const {
        return Vector2D(x + other.x, y + other.y);
    }

    Vector2D operator-(const Vector2D& other) const {
        return Vector2D(x - other.x, y - other.y);
    }

    Vector2D operator*(double scalar) const {
        return Vector2D(x * scalar, y * scalar);
    }
};

//------------------------------------------------------------------------------
// Normal calculation method
//------------------------------------------------------------------------------

enum class NormalMethod {
    Radial,    // Simple: vector from center to pixel
    Gradient,  // Distance map gradient
    Sobel,     // Smoothed Sobel gradient (best quality)
    Contour    // Edge-aware: normals perpendicular to local edge direction
};

//------------------------------------------------------------------------------
// Shading mode
//------------------------------------------------------------------------------

enum class ShadingMode {
    ThreeShade,  // Shadow | Base | Highlight
    FiveShade,   // Shadow | MidShadow | Base | MidHighlight | Highlight
    Gradient     // Smooth interpolation
};

//------------------------------------------------------------------------------
// Material type
//------------------------------------------------------------------------------

enum class MaterialType {
    Matte,    // Diffuse only, no specular
    Glossy,   // With specular highlights
    Textured  // Adds subtle noise/dithering
};

//------------------------------------------------------------------------------
// Fill mode for region detection
//------------------------------------------------------------------------------

enum class FillMode {
    SameColor,        // Fill pixels matching the clicked color (bucket tool behavior)
    AllNonTransparent, // Fill all connected non-transparent pixels regardless of color
    BoundedArea       // Fill entire bounded area (interior + boundary)
};

//------------------------------------------------------------------------------
// Shading configuration
//------------------------------------------------------------------------------

struct ShadeConfig {
    // Colors
    doc::color_t baseColor;
    doc::color_t shadowColor;
    doc::color_t highlightColor;
    doc::color_t midShadowColor;     // Optional for 5-shade
    doc::color_t midHighlightColor;  // Optional for 5-shade
    doc::color_t specularColor;      // For glossy materials

    // Light settings
    double lightAngle;      // 0-360 degrees
    double ambientLevel;    // 0.0-1.0 (typically 0.1-0.3)
    double lightElevation;  // 0.0-1.0: how much light comes from "above" (Z component)

    // Shape curvature
    double roundness;       // 0.0-2.0: controls spherical vs flat shading
                            // 0.0 = flat (linear gradient)
                            // 1.0 = hemisphere (true sphere)
                            // 2.0 = exaggerated roundness

    // Shading parameters
    ShadingMode shadingMode;
    NormalMethod normalMethod;
    MaterialType materialType;
    FillMode fillMode;      // How to detect the region to shade

    // Advanced options
    int colorTolerance;     // For flood fill (0-255)
    bool preventPillowShading;
    bool enableRimLight;
    bool enableSpecular;
    bool showOutline;       // Show selection outline in preview
    bool enableReflectedLight;  // Enable reflected light on shadow-side edges
    double rimLightIntensity;
    double specularShininess;
    double specularThreshold;
    int reflectedLightWidth;    // Width in pixels for reflected light strip (1-5)

    // Default constructor with sensible defaults
    ShadeConfig()
        : baseColor(doc::rgba(128, 128, 128, 255))      // Grey base
        , shadowColor(doc::rgba(64, 64, 64, 255))       // Dark grey shadow
        , highlightColor(doc::rgba(200, 200, 200, 255)) // Light grey highlight
        , midShadowColor(doc::rgba(96, 96, 96, 255))
        , midHighlightColor(doc::rgba(164, 164, 164, 255))
        , specularColor(doc::rgba(255, 255, 255, 255))
        , lightAngle(135.0)           // Top-left light (classic pixel art)
        , ambientLevel(0.15)          // Low ambient for more contrast
        , lightElevation(0.5)         // Light partially from above
        , roundness(1.0)              // Default hemisphere (sphere-like)
        , shadingMode(ShadingMode::ThreeShade)
        , normalMethod(NormalMethod::Gradient)
        , materialType(MaterialType::Matte)
        , fillMode(FillMode::AllNonTransparent)
        , colorTolerance(32)
        , preventPillowShading(true)
        , enableRimLight(false)
        , enableSpecular(false)
        , showOutline(true)
        , enableReflectedLight(true)  // Enable reflected light by default
        , rimLightIntensity(0.3)
        , specularShininess(32.0)
        , specularThreshold(0.8)
        , reflectedLightWidth(2)      // 2 pixels wide by default
    {}
};

//------------------------------------------------------------------------------
// Shape data structure
//------------------------------------------------------------------------------

struct ShapeData {
    std::vector<gfx::Point> pixels;      // All pixels in the region
    PointSet pixelSet;                   // Fast lookup set
    std::vector<gfx::Point> edgePixels;  // Boundary pixels
    DistanceMap distanceMap;             // Distance from each pixel to edge

    gfx::Rect bounds;                    // Bounding box
    double centerX;                      // Center of mass X
    double centerY;                      // Center of mass Y
    double maxDistance;                  // Maximum distance to edge

    ShapeData()
        : centerX(0)
        , centerY(0)
        , maxDistance(0)
    {}

    bool isEmpty() const { return pixels.empty(); }
    int pixelCount() const { return static_cast<int>(pixels.size()); }

    bool contains(const gfx::Point& p) const {
        return pixelSet.find(p) != pixelSet.end();
    }

    float getDistance(const gfx::Point& p) const {
        auto it = distanceMap.find(p);
        return (it != distanceMap.end()) ? it->second : 0.0f;
    }
};

//------------------------------------------------------------------------------
// Utility functions
//------------------------------------------------------------------------------

inline double clamp(double value, double min, double max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

inline int clampInt(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

inline double lerp(double a, double b, double t) {
    return a + t * (b - a);
}

// Convert angle in degrees to light direction vector
inline Vector2D angleToLightDirection(double angleDegrees) {
    double angleRad = angleDegrees * (3.14159265358979323846 / 180.0);
    // Note: Y is inverted in screen coordinates (Y increases downward)
    return Vector2D(std::cos(angleRad), -std::sin(angleRad));
}

} // namespace tools
} // namespace app

#endif // APP_TOOLS_AUTO_SHADE_SHADE_TYPES_H
