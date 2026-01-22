// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Normal Calculator Implementation

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "normal_calculator.h"

#include <cmath>
#include <cstdio>
#include <limits>

// Debug logging
#define SHADE_LOG(fmt, ...) printf("[AutoShade] " fmt "\n", ##__VA_ARGS__)

namespace app {
namespace tools {

NormalMap NormalCalculator::calculate(const ShapeData& shape, NormalMethod method)
{
    NormalMap normals;

    for (const auto& pixel : shape.pixels) {
        normals[pixel] = calculateSingle(pixel, shape, method);
    }

    return normals;
}

Vector2D NormalCalculator::calculateSingle(
    const gfx::Point& pixel,
    const ShapeData& shape,
    NormalMethod method)
{
    switch (method) {
        case NormalMethod::Radial:
            return calculateRadial(pixel, shape.centerX, shape.centerY);

        case NormalMethod::Gradient:
            return calculateGradient(pixel, shape);

        case NormalMethod::Contour:
            return calculateContour(pixel, shape);

        case NormalMethod::Sobel:
        default:
            return calculateSobel(pixel, shape);
    }
}

Vector2D NormalCalculator::calculateRadial(
    const gfx::Point& pixel,
    double centerX,
    double centerY)
{
    double dx = pixel.x - centerX;
    double dy = pixel.y - centerY;

    // Handle center pixel
    if (dx == 0 && dy == 0) {
        return Vector2D(0, -1);  // Default: point up
    }

    return Vector2D(dx, dy).normalized();
}

Vector2D NormalCalculator::calculateGradient(
    const gfx::Point& pixel,
    const ShapeData& shape)
{
    // If shape has no interior (all edge pixels, like an outline),
    // fall back to radial method which works better for outlines
    if (shape.maxDistance <= 0) {
        return calculateRadial(pixel, shape.centerX, shape.centerY);
    }

    // Get distances in 4 directions
    float distRight = getDistance(gfx::Point(pixel.x + 1, pixel.y), shape);
    float distLeft  = getDistance(gfx::Point(pixel.x - 1, pixel.y), shape);
    float distDown  = getDistance(gfx::Point(pixel.x, pixel.y + 1), shape);
    float distUp    = getDistance(gfx::Point(pixel.x, pixel.y - 1), shape);

    // Calculate gradient pointing OUTWARD (opposite of increasing distance)
    // Increasing distance points toward center, so negate to point outward
    // This makes it consistent with radial method: (pixel - center)
    double gradX = distLeft - distRight;  // Negate: point away from center
    double gradY = distUp - distDown;     // Negate: point away from center

    // Handle zero gradient - fall back to radial
    if (gradX == 0 && gradY == 0) {
        return calculateRadial(pixel, shape.centerX, shape.centerY);
    }

    return Vector2D(gradX, gradY).normalized();
}

Vector2D NormalCalculator::calculateSobel(
    const gfx::Point& pixel,
    const ShapeData& shape)
{
    // If shape has no interior (all edge pixels, like an outline),
    // fall back to radial method which works better for outlines
    if (shape.maxDistance <= 0) {
        return calculateRadial(pixel, shape.centerX, shape.centerY);
    }

    // Get 3x3 neighborhood distances
    // Layout:
    //   TL  T  TR
    //   L   C  R
    //   BL  B  BR

    float dTL = getDistance(gfx::Point(pixel.x - 1, pixel.y - 1), shape);
    float dT  = getDistance(gfx::Point(pixel.x,     pixel.y - 1), shape);
    float dTR = getDistance(gfx::Point(pixel.x + 1, pixel.y - 1), shape);
    float dL  = getDistance(gfx::Point(pixel.x - 1, pixel.y),     shape);
    float dR  = getDistance(gfx::Point(pixel.x + 1, pixel.y),     shape);
    float dBL = getDistance(gfx::Point(pixel.x - 1, pixel.y + 1), shape);
    float dB  = getDistance(gfx::Point(pixel.x,     pixel.y + 1), shape);
    float dBR = getDistance(gfx::Point(pixel.x + 1, pixel.y + 1), shape);

    // Sobel operator for X gradient (standard computes direction of increase):
    // [-1  0  +1]
    // [-2  0  +2]
    // [-1  0  +1]
    // NEGATE to point outward (away from center/max distance)
    double gradX = (dTL + 2.0 * dL + dBL) - (dTR + 2.0 * dR + dBR);

    // Sobel operator for Y gradient:
    // [-1 -2 -1]
    // [ 0  0  0]
    // [+1 +2 +1]
    // NEGATE to point outward (away from center/max distance)
    double gradY = (dTL + 2.0 * dT + dTR) - (dBL + 2.0 * dB + dBR);

    // Handle zero gradient - fall back to radial
    if (gradX == 0 && gradY == 0) {
        return calculateRadial(pixel, shape.centerX, shape.centerY);
    }

    return Vector2D(gradX, gradY).normalized();
}

float NormalCalculator::getDistance(
    const gfx::Point& pixel,
    const ShapeData& shape)
{
    // If pixel is in the region, return its distance
    if (shape.contains(pixel)) {
        return shape.getDistance(pixel);
    }

    // Outside the region = edge (distance 0)
    return 0.0f;
}

std::unordered_map<gfx::Point, Vector2D, PointHash> NormalCalculator::calculateEdgeNormals(
    const ShapeData& shape)
{
    std::unordered_map<gfx::Point, Vector2D, PointHash> edgeNormals;

    if (shape.edgePixels.empty()) {
        SHADE_LOG("calculateEdgeNormals: no edge pixels found");
        return edgeNormals;
    }

    SHADE_LOG("calculateEdgeNormals: processing %zu edge pixels, center=(%.1f, %.1f)",
              shape.edgePixels.size(), shape.centerX, shape.centerY);

    // For each edge pixel, calculate the local tangent by looking at
    // neighboring edge pixels, then compute normal perpendicular to tangent
    PointSet edgeSet(shape.edgePixels.begin(), shape.edgePixels.end());

    // 8-connected neighbor offsets for finding edge neighbors
    const gfx::Point offsets[8] = {
        gfx::Point(1, 0), gfx::Point(1, 1), gfx::Point(0, 1), gfx::Point(-1, 1),
        gfx::Point(-1, 0), gfx::Point(-1, -1), gfx::Point(0, -1), gfx::Point(1, -1)
    };

    int tangentBased = 0;
    int singleNeighbor = 0;
    int radialFallback = 0;

    for (const auto& edgePixel : shape.edgePixels) {
        // Find neighboring edge pixels to determine local tangent direction
        std::vector<gfx::Point> neighbors;
        for (int i = 0; i < 8; ++i) {
            gfx::Point neighbor(edgePixel.x + offsets[i].x, edgePixel.y + offsets[i].y);
            if (edgeSet.find(neighbor) != edgeSet.end()) {
                neighbors.push_back(neighbor);
            }
        }

        Vector2D normal;
        if (neighbors.size() >= 2) {
            // Calculate tangent as direction between neighbors
            // Use average of all neighbor directions for smoother result
            Vector2D avgTangent(0, 0);
            for (size_t i = 0; i < neighbors.size(); ++i) {
                for (size_t j = i + 1; j < neighbors.size(); ++j) {
                    double dx = neighbors[j].x - neighbors[i].x;
                    double dy = neighbors[j].y - neighbors[i].y;
                    avgTangent = avgTangent + Vector2D(dx, dy);
                }
            }
            avgTangent = avgTangent.normalized();

            // Normal is perpendicular to tangent
            // Choose direction pointing OUTWARD (away from center) for lighting
            normal = Vector2D(-avgTangent.y, avgTangent.x);

            // Make sure normal points OUTWARD (away from center of shape)
            Vector2D toCenter(shape.centerX - edgePixel.x, shape.centerY - edgePixel.y);
            if (normal.dot(toCenter) > 0) {  // If pointing toward center, flip it
                normal = Vector2D(avgTangent.y, -avgTangent.x);
            }
            tangentBased++;
        }
        else if (neighbors.size() == 1) {
            // Only one neighbor - use direction from neighbor to this pixel
            Vector2D tangent(edgePixel.x - neighbors[0].x, edgePixel.y - neighbors[0].y);
            tangent = tangent.normalized();
            normal = Vector2D(-tangent.y, tangent.x);

            // Point OUTWARD (away from center)
            Vector2D toCenter(shape.centerX - edgePixel.x, shape.centerY - edgePixel.y);
            if (normal.dot(toCenter) > 0) {  // If pointing toward center, flip it
                normal = Vector2D(tangent.y, -tangent.x);
            }
            singleNeighbor++;
        }
        else {
            // No neighbors - use radial normal from center (already points outward)
            normal = calculateRadial(edgePixel, shape.centerX, shape.centerY);
            radialFallback++;
        }

        edgeNormals[edgePixel] = normal.normalized();
    }

    SHADE_LOG("calculateEdgeNormals: tangent-based=%d, single-neighbor=%d, radial-fallback=%d",
              tangentBased, singleNeighbor, radialFallback);

    return edgeNormals;
}

Vector2D NormalCalculator::findClosestEdgeNormal(
    const gfx::Point& pixel,
    const ShapeData& shape,
    const std::unordered_map<gfx::Point, Vector2D, PointHash>& edgeNormals)
{
    if (edgeNormals.empty()) {
        return calculateRadial(pixel, shape.centerX, shape.centerY);
    }

    // Find the closest edge pixel
    double minDistSq = std::numeric_limits<double>::max();
    Vector2D closestNormal(0, -1);

    for (const auto& edgePixel : shape.edgePixels) {
        double dx = pixel.x - edgePixel.x;
        double dy = pixel.y - edgePixel.y;
        double distSq = dx * dx + dy * dy;

        if (distSq < minDistSq) {
            minDistSq = distSq;
            auto it = edgeNormals.find(edgePixel);
            if (it != edgeNormals.end()) {
                closestNormal = it->second;
            }
        }
    }

    return closestNormal;
}

Vector2D NormalCalculator::calculateContour(
    const gfx::Point& pixel,
    const ShapeData& shape)
{
    // For edge pixels: use tangent-based normals perpendicular to local edge
    // For interior pixels: use gradient-based normals (smooth, no artifacts)

    // Check if this is an edge pixel
    PointSet edgeSet(shape.edgePixels.begin(), shape.edgePixels.end());
    bool isEdge = edgeSet.find(pixel) != edgeSet.end();

    // Static counters for logging (reset when shape changes)
    static const ShapeData* loggedShape = nullptr;
    static int edgePixelCount = 0;
    static int interiorPixelCount = 0;

    if (loggedShape != &shape) {
        // New shape - log previous shape's summary if exists and reset
        if (loggedShape != nullptr) {
            SHADE_LOG("calculateContour summary: edge=%d, interior=%d", edgePixelCount, interiorPixelCount);
        }
        loggedShape = &shape;
        edgePixelCount = 0;
        interiorPixelCount = 0;
        SHADE_LOG("calculateContour: starting for shape with %zu pixels, maxDist=%.1f",
                  shape.pixels.size(), shape.maxDistance);
    }

    if (isEdge) {
        edgePixelCount++;
        // Calculate edge normal directly using tangent-based method
        static std::unordered_map<gfx::Point, Vector2D, PointHash> cachedEdgeNormals;
        static const ShapeData* cachedShape = nullptr;

        // Cache edge normals for the current shape
        if (cachedShape != &shape) {
            cachedEdgeNormals = calculateEdgeNormals(shape);
            cachedShape = &shape;
        }

        auto it = cachedEdgeNormals.find(pixel);
        if (it != cachedEdgeNormals.end()) {
            return it->second;
        }
    }

    interiorPixelCount++;

    // For interior pixels: use gradient-based normals for smooth results
    // This avoids the Voronoi-like artifacts from closest-edge lookup
    return calculateGradient(pixel, shape);
}

} // namespace tools
} // namespace app
