// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Main Tool Implementation

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "auto_shade_tool.h"
#include "doc/primitives.h"

namespace app {
namespace tools {

AutoShadeTool::AutoShadeTool()
{
}

AutoShadeTool::~AutoShadeTool()
{
}

bool AutoShadeTool::applyShading(
    doc::Image* image,
    int startX,
    int startY)
{
    if (!image) {
        return false;
    }

    // Analyze the region first
    if (!analyzeRegion(image, startX, startY)) {
        return false;
    }

    // Calculate light direction from angle
    m_lightDirection = angleToLightDirection(m_config.lightAngle);

    // Calculate normals for all pixels
    m_lastNormals = NormalCalculator::calculate(m_lastShape, m_config.normalMethod);

    // Create a set of edge pixels for rim light calculation
    PointSet edgeSet;
    for (const auto& edge : m_lastShape.edgePixels) {
        edgeSet.insert(edge);
    }

    // Apply shading to each pixel
    for (const auto& pixel : m_lastShape.pixels) {
        // Get normal for this pixel
        auto normalIt = m_lastNormals.find(pixel);
        if (normalIt == m_lastNormals.end()) {
            continue;
        }

        const Vector2D& normal = normalIt->second;
        bool isEdge = (edgeSet.find(pixel) != edgeSet.end());

        // Calculate shaded color
        doc::color_t newColor = ShadingCalculator::shade(
            normal,
            m_lightDirection,
            isEdge,
            m_config);

        // Apply to image
        doc::put_pixel(image, pixel.x, pixel.y, newColor);
    }

    return true;
}

bool AutoShadeTool::analyzeRegion(
    const doc::Image* image,
    int startX,
    int startY,
    const doc::Palette* palette)
{
    if (!image) {
        return false;
    }

    // Flood fill to get region
    // Pass palette for indexed images to compare actual RGB colors
    // Pass fillMode to control how region detection works
    std::vector<gfx::Point> regionPixels = FloodFill::fillFromPoint(
        image, startX, startY, m_config.colorTolerance, palette, m_config.fillMode);

    if (regionPixels.empty()) {
        m_lastShape = ShapeData();
        return false;
    }

    // Analyze shape
    m_lastShape = ShapeAnalyzer::analyze(regionPixels);

    return true;
}

std::unordered_map<gfx::Point, doc::color_t, PointHash> AutoShadeTool::generatePreview()
{
    std::unordered_map<gfx::Point, doc::color_t, PointHash> preview;

    if (m_lastShape.isEmpty()) {
        return preview;
    }

    // Calculate light direction from angle
    m_lightDirection = angleToLightDirection(m_config.lightAngle);

    // Calculate normals for all pixels
    m_lastNormals = NormalCalculator::calculate(m_lastShape, m_config.normalMethod);

    // Create a set of edge pixels for rim light calculation
    PointSet edgeSet;
    for (const auto& edge : m_lastShape.edgePixels) {
        edgeSet.insert(edge);
    }

    // Generate shaded color for each pixel
    for (const auto& pixel : m_lastShape.pixels) {
        auto normalIt = m_lastNormals.find(pixel);
        if (normalIt == m_lastNormals.end()) {
            continue;
        }

        const Vector2D& normal = normalIt->second;
        bool isEdge = (edgeSet.find(pixel) != edgeSet.end());

        doc::color_t newColor = ShadingCalculator::shade(
            normal,
            m_lightDirection,
            isEdge,
            m_config);

        preview[pixel] = newColor;
    }

    return preview;
}

void AutoShadeTool::reset()
{
    m_lastShape = ShapeData();
    m_lastNormals.clear();
    m_lightDirection = Vector2D(0, -1);
}

} // namespace tools
} // namespace app
