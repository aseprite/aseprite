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
// Shape type for normal calculation
//------------------------------------------------------------------------------

enum class ShapeType {
    Sphere,      // Perfectly smooth spherical normals (radial from center)
    Adaptive,    // Follow actual shape silhouette (uses distance map)
    Cylinder,    // Cylindrical shading (horizontal or vertical axis)
    Flat         // Flat plane facing viewer (uniform lighting)
};

//------------------------------------------------------------------------------
// Shading style - how colors are arranged/applied
//------------------------------------------------------------------------------

enum class ShadingStyle {
    ClassicCartoon,    // Hard edges, solid color zones
    SoftCartoon,       // Softer edges, light AA transitions
    OilSoft,           // Painterly, irregular texture
    RawPaint,          // Smooth dithered gradients
    Dotted,            // Screen-tone dot pattern
    StrokeSphere,      // Concentric circular strokes
    StrokeVertical,    // Vertical hatching lines
    StrokeHorizontal,  // Horizontal hatching lines
    SmallGrain,        // Fine noise texture
    LargeGrain,        // Chunky noise clusters
    TrickyShading,     // Complex mixed patterns
    SoftPattern,       // Perlin noise overlay
    Wrinkled,          // Flow-based wrinkle lines
    Patterned,         // Regular geometric patterns (scales, cells)
    Wood,              // Wood grain rings/lines
    HardBrush          // Visible brush strokes
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
// Color source for shading
//------------------------------------------------------------------------------

enum class ColorSource {
    Foreground,   // Derive colors from foreground color (default)
    Background    // Derive colors from background color
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
    double lightAngle;      // 0-360 degrees (horizontal direction)
    double ambientLevel;    // 0.0-1.0 (typically 0.1-0.3)
    double lightElevation;  // 0-180 degrees: angle of light from horizontal plane
                            // 0° = front (horizontal), 90° = top, 180° = back

    // Shape curvature
    double roundness;       // 0.0-2.0: controls spherical vs flat shading
                            // 0.0 = flat (linear gradient)
                            // 1.0 = hemisphere (true sphere)
                            // 2.0 = exaggerated roundness

    // Shading parameters
    ShadingMode shadingMode;
    NormalMethod normalMethod;
    MaterialType materialType;
    ShapeType shapeType;    // Shape assumption for normal calculation
    FillMode fillMode;      // How to detect the region to shade
    ColorSource colorSource; // Where to get base color from (foreground/background)
    ShadingStyle shadingStyle; // How colors are arranged/applied (style effect)

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

    // Selective outlining - different outline colors for light/shadow sides
    bool enableSelectiveOutline;     // Use different outline colors based on light
    doc::color_t lightOutlineColor;  // Outline color on light-facing side
    doc::color_t shadowOutlineColor; // Outline color on shadow-facing side

    // Highlight control
    double highlightFocus;  // 0.0-1.0: how concentrated/small the highlight is
                            // 0.0 = spread out, 1.0 = very focused/small

    // Anti-aliasing
    bool enableAntiAliasing;    // Add AA around silhouette edges
    int aaLevels;               // Number of AA levels (1-3)

    // Band transition smoothing
    bool enableDithering;       // Add dithering at band boundaries for smoother transitions
    int ditheringWidth;         // Width of dithering zone in pixels (1-3)

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
        , lightElevation(45.0)        // 45° elevation (between front and top)
        , roundness(1.0)              // Default hemisphere (sphere-like)
        , shadingMode(ShadingMode::ThreeShade)
        , normalMethod(NormalMethod::Gradient)
        , materialType(MaterialType::Matte)
        , shapeType(ShapeType::Sphere)        // Default: smooth sphere (best for most pixel art)
        , fillMode(FillMode::AllNonTransparent)
        , colorSource(ColorSource::Foreground)  // Default: use foreground color
        , shadingStyle(ShadingStyle::ClassicCartoon) // Default: clean solid zones
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
        , enableSelectiveOutline(true) // Selective outline enabled by default
        , lightOutlineColor(doc::rgba(255, 165, 0, 255))   // Bright orange for light side
        , shadowOutlineColor(doc::rgba(139, 0, 0, 255))    // Dark red for shadow side
        , highlightFocus(0.5)         // Medium focus by default
        , enableAntiAliasing(false)   // AA disabled by default (optional feature)
        , aaLevels(2)                 // 2 levels of AA
        , enableDithering(false)      // Dithering disabled by default
        , ditheringWidth(1)           // 1 pixel dithering zone
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
