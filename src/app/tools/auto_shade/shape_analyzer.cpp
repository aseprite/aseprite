// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Shape Analyzer Implementation

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "shape_analyzer.h"

#include <queue>
#include <limits>

namespace app {
namespace tools {

ShapeData ShapeAnalyzer::analyze(const std::vector<gfx::Point>& regionPixels)
{
    ShapeData shape;

    if (regionPixels.empty()) {
        return shape;
    }

    // Copy pixels and create lookup set
    shape.pixels = regionPixels;
    for (const auto& pixel : regionPixels) {
        shape.pixelSet.insert(pixel);
    }

    // Calculate bounds and center
    calculateBoundsAndCenter(regionPixels, shape);

    // Find edge pixels
    findEdgePixels(shape);

    // Create distance map
    createDistanceMap(shape);

    return shape;
}

void ShapeAnalyzer::calculateBoundsAndCenter(
    const std::vector<gfx::Point>& pixels,
    ShapeData& shape)
{
    if (pixels.empty()) {
        shape.bounds = gfx::Rect(0, 0, 0, 0);
        shape.centerX = 0;
        shape.centerY = 0;
        return;
    }

    int minX = std::numeric_limits<int>::max();
    int maxX = std::numeric_limits<int>::min();
    int minY = std::numeric_limits<int>::max();
    int maxY = std::numeric_limits<int>::min();

    double sumX = 0;
    double sumY = 0;

    for (const auto& pixel : pixels) {
        if (pixel.x < minX) minX = pixel.x;
        if (pixel.x > maxX) maxX = pixel.x;
        if (pixel.y < minY) minY = pixel.y;
        if (pixel.y > maxY) maxY = pixel.y;

        sumX += pixel.x;
        sumY += pixel.y;
    }

    shape.bounds = gfx::Rect(minX, minY, maxX - minX + 1, maxY - minY + 1);
    shape.centerX = sumX / pixels.size();
    shape.centerY = sumY / pixels.size();
}

void ShapeAnalyzer::findEdgePixels(ShapeData& shape)
{
    shape.edgePixels.clear();

    // 4-connected neighbor offsets
    const gfx::Point offsets[4] = {
        gfx::Point(1, 0),   // Right
        gfx::Point(-1, 0),  // Left
        gfx::Point(0, 1),   // Down
        gfx::Point(0, -1)   // Up
    };

    for (const auto& pixel : shape.pixels) {
        bool isEdge = false;

        // Check each neighbor
        for (int i = 0; i < 4; ++i) {
            gfx::Point neighbor(pixel.x + offsets[i].x, pixel.y + offsets[i].y);

            // If neighbor is not in the region, this is an edge pixel
            if (!shape.contains(neighbor)) {
                isEdge = true;
                break;
            }
        }

        if (isEdge) {
            shape.edgePixels.push_back(pixel);
        }
    }
}

void ShapeAnalyzer::createDistanceMap(ShapeData& shape)
{
    shape.distanceMap.clear();
    shape.maxDistance = 0;

    if (shape.edgePixels.empty()) {
        // All pixels are edges (single pixel or line)
        for (const auto& pixel : shape.pixels) {
            shape.distanceMap[pixel] = 0.0f;
        }
        return;
    }

    // Initialize: edge pixels have distance 0
    std::queue<gfx::Point> queue;
    for (const auto& pixel : shape.edgePixels) {
        shape.distanceMap[pixel] = 0.0f;
        queue.push(pixel);
    }

    // 4-connected neighbor offsets
    const gfx::Point offsets[4] = {
        gfx::Point(1, 0),
        gfx::Point(-1, 0),
        gfx::Point(0, 1),
        gfx::Point(0, -1)
    };

    // BFS from all edge pixels simultaneously
    while (!queue.empty()) {
        gfx::Point current = queue.front();
        queue.pop();

        float currentDist = shape.distanceMap[current];

        // Check each neighbor
        for (int i = 0; i < 4; ++i) {
            gfx::Point neighbor(current.x + offsets[i].x, current.y + offsets[i].y);

            // Check if neighbor is in the region and not yet visited
            if (shape.contains(neighbor) &&
                shape.distanceMap.find(neighbor) == shape.distanceMap.end()) {

                float newDist = currentDist + 1.0f;
                shape.distanceMap[neighbor] = newDist;

                if (newDist > shape.maxDistance) {
                    shape.maxDistance = newDist;
                }

                queue.push(neighbor);
            }
        }
    }
}

} // namespace tools
} // namespace app
