// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Normal Calculator
//
// Calculates surface normals for each pixel using various methods:
// - Radial: Simple vector from center (good for convex shapes)
// - Gradient: Distance map gradient (better for complex shapes)
// - Sobel: Smoothed gradient using Sobel operator (best quality)
// - Contour: Edge-aware normals perpendicular to local edge direction

#ifndef APP_TOOLS_AUTO_SHADE_NORMAL_CALCULATOR_H
#define APP_TOOLS_AUTO_SHADE_NORMAL_CALCULATOR_H

#include "shade_types.h"

#include <unordered_map>

namespace app {
namespace tools {

using NormalMap = std::unordered_map<gfx::Point, Vector2D, PointHash>;

class NormalCalculator {
public:
    // Calculate normals for all pixels in the shape
    static NormalMap calculate(const ShapeData& shape, NormalMethod method);

    // Calculate normal for a single pixel using specified method
    static Vector2D calculateSingle(
        const gfx::Point& pixel,
        const ShapeData& shape,
        NormalMethod method);

private:
    // Radial normal: vector from center to pixel
    static Vector2D calculateRadial(
        const gfx::Point& pixel,
        double centerX,
        double centerY);

    // Gradient normal: based on distance map gradient
    static Vector2D calculateGradient(
        const gfx::Point& pixel,
        const ShapeData& shape);

    // Sobel normal: smoothed gradient using 3x3 Sobel operator
    static Vector2D calculateSobel(
        const gfx::Point& pixel,
        const ShapeData& shape);

    // Contour normal: perpendicular to local edge direction
    static Vector2D calculateContour(
        const gfx::Point& pixel,
        const ShapeData& shape);

    // Calculate edge normals for all edge pixels
    static std::unordered_map<gfx::Point, Vector2D, PointHash> calculateEdgeNormals(
        const ShapeData& shape);

    // Find closest edge pixel and its normal
    static Vector2D findClosestEdgeNormal(
        const gfx::Point& pixel,
        const ShapeData& shape,
        const std::unordered_map<gfx::Point, Vector2D, PointHash>& edgeNormals);

    // Helper: get distance at a point (returns 0 if outside region)
    static float getDistance(
        const gfx::Point& pixel,
        const ShapeData& shape);
};

} // namespace tools
} // namespace app

#endif // APP_TOOLS_AUTO_SHADE_NORMAL_CALCULATOR_H
