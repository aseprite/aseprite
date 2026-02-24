// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Flood Fill Algorithm
//
// BFS-based flood fill for detecting connected regions of similar color.

#ifndef APP_TOOLS_AUTO_SHADE_FLOOD_FILL_H
#define APP_TOOLS_AUTO_SHADE_FLOOD_FILL_H

#include "shade_types.h"
#include "doc/image.h"
#include "doc/color.h"
#include "doc/palette.h"

#include <vector>

namespace app {
namespace tools {

class FloodFill {
public:
    // Fill from a starting point, returning all connected pixels
    // that match the target color within tolerance
    // For indexed images, palette is used to compare actual RGB colors
    static std::vector<gfx::Point> fill(
        const doc::Image* image,
        int startX,
        int startY,
        doc::color_t targetColor,
        int tolerance = 0,
        const doc::Palette* palette = nullptr,
        FillMode fillMode = FillMode::SameColor);

    // Fill from a starting point, using the color at that position
    // For indexed images, palette is used to compare actual RGB colors
    static std::vector<gfx::Point> fillFromPoint(
        const doc::Image* image,
        int startX,
        int startY,
        int tolerance = 0,
        const doc::Palette* palette = nullptr,
        FillMode fillMode = FillMode::SameColor);

    // Fill all non-transparent pixels connected to the starting point
    static std::vector<gfx::Point> fillAllNonTransparent(
        const doc::Image* image,
        int startX,
        int startY,
        const doc::Palette* palette = nullptr);

    // Fill bounded area - fills all pixels (including transparent) until hitting
    // the boundary of the image or completely transparent regions
    static std::vector<gfx::Point> fillBoundedArea(
        const doc::Image* image,
        int startX,
        int startY,
        const doc::Palette* palette = nullptr);

private:
    // Calculate distance between two colors in RGB space
    static double colorDistance(doc::color_t c1, doc::color_t c2);

    // Check if a color matches within tolerance
    static bool colorMatches(doc::color_t c1, doc::color_t c2, int tolerance);

    // Get RGB color from indexed image using palette
    static doc::color_t getIndexedColor(const doc::Palette* palette, doc::color_t index);

    // Check if a pixel is transparent (helper)
    static bool isTransparent(const doc::Image* image, int x, int y, const doc::Palette* palette);
};

} // namespace tools
} // namespace app

#endif // APP_TOOLS_AUTO_SHADE_FLOOD_FILL_H
