// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Shape Analyzer
//
// Analyzes shape geometry: bounds, center, edges, and distance map.

#ifndef APP_TOOLS_AUTO_SHADE_SHAPE_ANALYZER_H
#define APP_TOOLS_AUTO_SHADE_SHAPE_ANALYZER_H

#include "shade_types.h"

#include <vector>

namespace app {
namespace tools {

class ShapeAnalyzer {
public:
    // Analyze a region and return shape data
    static ShapeData analyze(const std::vector<gfx::Point>& regionPixels);

private:
    // Calculate bounding box and center of mass
    static void calculateBoundsAndCenter(
        const std::vector<gfx::Point>& pixels,
        ShapeData& shape);

    // Find edge pixels (pixels with at least one neighbor outside the region)
    static void findEdgePixels(ShapeData& shape);

    // Create distance map using BFS from edge pixels
    static void createDistanceMap(ShapeData& shape);
};

} // namespace tools
} // namespace app

#endif // APP_TOOLS_AUTO_SHADE_SHAPE_ANALYZER_H
