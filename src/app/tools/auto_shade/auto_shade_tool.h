// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Main Tool Class
//
// Combines all components to provide automatic pixel art shading.

#ifndef APP_TOOLS_AUTO_SHADE_AUTO_SHADE_TOOL_H
#define APP_TOOLS_AUTO_SHADE_AUTO_SHADE_TOOL_H

#include "shade_types.h"
#include "flood_fill.h"
#include "shape_analyzer.h"
#include "normal_calculator.h"
#include "shading_calculator.h"

#include "doc/image.h"
#include "doc/color.h"
#include "doc/palette.h"

namespace app {
namespace tools {

class AutoShadeTool {
public:
    AutoShadeTool();
    ~AutoShadeTool();

    //--------------------------------------------------------------------------
    // Configuration
    //--------------------------------------------------------------------------

    /// Get current configuration
    const ShadeConfig& config() const { return m_config; }

    /// Get mutable configuration
    ShadeConfig& config() { return m_config; }

    /// Set configuration
    void setConfig(const ShadeConfig& config) { m_config = config; }

    //--------------------------------------------------------------------------
    // Shading Operations
    //--------------------------------------------------------------------------

    /// Apply shading to an image at the specified point
    /// Returns true if shading was applied successfully
    bool applyShading(
        doc::Image* image,
        int startX,
        int startY);

    /// Get the last detected shape data (for preview)
    const ShapeData& lastShape() const { return m_lastShape; }

    /// Get mutable shape data (for coordinate adjustments)
    ShapeData& lastShape() { return m_lastShape; }

    /// Check if there's shape data available
    bool hasShapeData() const { return !m_lastShape.isEmpty(); }

    //--------------------------------------------------------------------------
    // Preview Generation
    //--------------------------------------------------------------------------

    /// Analyze a region without applying shading (for preview)
    /// For indexed images, pass the palette for proper color comparison
    bool analyzeRegion(
        const doc::Image* image,
        int startX,
        int startY,
        const doc::Palette* palette = nullptr);

    /// Generate preview of shaded pixels without modifying image
    /// Returns map of point -> color
    std::unordered_map<gfx::Point, doc::color_t, PointHash> generatePreview();

    //--------------------------------------------------------------------------
    // Reset
    //--------------------------------------------------------------------------

    /// Clear all state
    void reset();

private:
    ShadeConfig m_config;
    ShapeData m_lastShape;
    NormalMap m_lastNormals;
    Vector2D m_lightDirection;
};

} // namespace tools
} // namespace app

#endif // APP_TOOLS_AUTO_SHADE_AUTO_SHADE_TOOL_H
