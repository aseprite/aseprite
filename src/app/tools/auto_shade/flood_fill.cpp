// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Flood Fill Implementation

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "flood_fill.h"
#include "doc/image.h"
#include "doc/primitives.h"

#include <queue>
#include <cmath>
#include <cstdio>

namespace app {
namespace tools {

// Debug logging
#define SHADE_LOG(fmt, ...) printf("[AutoShade] " fmt "\n", ##__VA_ARGS__)

std::vector<gfx::Point> FloodFill::fill(
    const doc::Image* image,
    int startX,
    int startY,
    doc::color_t targetColor,
    int tolerance,
    const doc::Palette* palette,
    FillMode fillMode)
{
    std::vector<gfx::Point> region;

    if (!image) {
        return region;
    }

    int width = image->width();
    int height = image->height();

    // Bounds check
    if (startX < 0 || startX >= width || startY < 0 || startY >= height) {
        return region;
    }

    // Get pixel format
    doc::PixelFormat format = image->pixelFormat();

    // For indexed images, convert target to RGB for comparison
    doc::color_t targetRgb = targetColor;
    if (format == doc::IMAGE_INDEXED && palette) {
        targetRgb = getIndexedColor(palette, targetColor);
    }

    // BFS queue
    std::queue<gfx::Point> queue;
    PointSet visited;

    queue.push(gfx::Point(startX, startY));

    while (!queue.empty()) {
        gfx::Point current = queue.front();
        queue.pop();

        // Skip if already visited
        if (visited.find(current) != visited.end()) {
            continue;
        }

        // Bounds check
        if (current.x < 0 || current.x >= width ||
            current.y < 0 || current.y >= height) {
            continue;
        }

        // Get pixel color
        doc::color_t pixelColor = doc::get_pixel(image, current.x, current.y);

        // Check for match based on pixel format
        bool matches = false;
        switch (format) {
            case doc::IMAGE_RGB:
                // Check alpha - skip fully transparent pixels
                if (doc::rgba_geta(pixelColor) == 0 && doc::rgba_geta(targetColor) != 0) {
                    continue;
                }
                matches = colorMatches(pixelColor, targetColor, tolerance);
                break;

            case doc::IMAGE_INDEXED:
                // For indexed images, compare actual RGB colors from palette
                if (palette) {
                    doc::color_t pixelRgb = getIndexedColor(palette, pixelColor);
                    // Check alpha - skip transparent palette entries
                    if (doc::rgba_geta(pixelRgb) == 0 && doc::rgba_geta(targetRgb) != 0) {
                        continue;
                    }
                    matches = colorMatches(pixelRgb, targetRgb, tolerance);
                } else {
                    // Fallback: compare indices directly
                    if (tolerance == 0) {
                        matches = (pixelColor == targetColor);
                    } else {
                        int diff = std::abs(static_cast<int>(pixelColor) - static_cast<int>(targetColor));
                        matches = (diff <= tolerance);
                    }
                }
                break;

            case doc::IMAGE_GRAYSCALE:
                // For grayscale, compare gray values
                if (doc::graya_geta(pixelColor) == 0 && doc::graya_geta(targetColor) != 0) {
                    continue;
                }
                if (tolerance == 0) {
                    matches = (doc::graya_getv(pixelColor) == doc::graya_getv(targetColor));
                } else {
                    int diff = std::abs(doc::graya_getv(pixelColor) - doc::graya_getv(targetColor));
                    matches = (diff <= tolerance);
                }
                break;

            default:
                // Unsupported format
                continue;
        }

        if (!matches) {
            continue;
        }

        // Mark as visited and add to region
        visited.insert(current);
        region.push_back(current);

        // Enqueue 4-connected neighbors
        queue.push(gfx::Point(current.x + 1, current.y));  // Right
        queue.push(gfx::Point(current.x - 1, current.y));  // Left
        queue.push(gfx::Point(current.x, current.y + 1));  // Down
        queue.push(gfx::Point(current.x, current.y - 1));  // Up
    }

    return region;
}

std::vector<gfx::Point> FloodFill::fillFromPoint(
    const doc::Image* image,
    int startX,
    int startY,
    int tolerance,
    const doc::Palette* palette,
    FillMode fillMode)
{
    const char* modeStr = (fillMode == FillMode::SameColor) ? "SameColor" :
                          (fillMode == FillMode::AllNonTransparent) ? "AllNonTransparent" : "BoundedArea";
    SHADE_LOG("fillFromPoint: start=(%d,%d) mode=%s", startX, startY, modeStr);

    if (!image) {
        SHADE_LOG("fillFromPoint: ERROR - no image");
        return std::vector<gfx::Point>();
    }

    int width = image->width();
    int height = image->height();
    SHADE_LOG("fillFromPoint: image size=%dx%d", width, height);

    // Bounds check
    if (startX < 0 || startX >= width || startY < 0 || startY >= height) {
        SHADE_LOG("fillFromPoint: ERROR - out of bounds");
        return std::vector<gfx::Point>();
    }

    // Handle different fill modes
    std::vector<gfx::Point> result;
    switch (fillMode) {
        case FillMode::AllNonTransparent:
            result = fillAllNonTransparent(image, startX, startY, palette);
            break;

        case FillMode::BoundedArea:
            result = fillBoundedArea(image, startX, startY, palette);
            break;

        case FillMode::SameColor:
        default:
            // Get target color from the starting point
            doc::color_t targetColor = doc::get_pixel(image, startX, startY);
            result = fill(image, startX, startY, targetColor, tolerance, palette, fillMode);
            break;
    }

    SHADE_LOG("fillFromPoint: found %zu pixels", result.size());
    return result;
}

std::vector<gfx::Point> FloodFill::fillAllNonTransparent(
    const doc::Image* image,
    int startX,
    int startY,
    const doc::Palette* palette)
{
    SHADE_LOG("fillAllNonTransparent: start=(%d,%d)", startX, startY);
    std::vector<gfx::Point> region;

    if (!image) {
        return region;
    }

    int width = image->width();
    int height = image->height();

    // Bounds check
    if (startX < 0 || startX >= width || startY < 0 || startY >= height) {
        return region;
    }

    // Check if start pixel is transparent
    bool startIsTransparent = isTransparent(image, startX, startY, palette);
    SHADE_LOG("fillAllNonTransparent: startIsTransparent=%d", startIsTransparent);

    // If clicking on transparent, try to detect bounded area
    // This handles the case of clicking inside an outline shape
    if (startIsTransparent) {
        SHADE_LOG("fillAllNonTransparent: delegating to fillBoundedArea");
        // Use bounded area detection - will return outline + interior if bounded
        return fillBoundedArea(image, startX, startY, palette);
    }

    // BFS queue - fill connected non-transparent pixels
    std::queue<gfx::Point> queue;
    PointSet visited;

    queue.push(gfx::Point(startX, startY));

    while (!queue.empty()) {
        gfx::Point current = queue.front();
        queue.pop();

        // Skip if already visited
        if (visited.find(current) != visited.end()) {
            continue;
        }

        // Bounds check
        if (current.x < 0 || current.x >= width ||
            current.y < 0 || current.y >= height) {
            continue;
        }

        // Skip transparent pixels
        if (isTransparent(image, current.x, current.y, palette)) {
            continue;
        }

        // Mark as visited and add to region
        visited.insert(current);
        region.push_back(current);

        // Enqueue 4-connected neighbors
        queue.push(gfx::Point(current.x + 1, current.y));
        queue.push(gfx::Point(current.x - 1, current.y));
        queue.push(gfx::Point(current.x, current.y + 1));
        queue.push(gfx::Point(current.x, current.y - 1));
    }

    return region;
}

std::vector<gfx::Point> FloodFill::fillBoundedArea(
    const doc::Image* image,
    int startX,
    int startY,
    const doc::Palette* palette)
{
    SHADE_LOG("fillBoundedArea: start=(%d,%d)", startX, startY);
    std::vector<gfx::Point> region;

    if (!image) {
        return region;
    }

    int width = image->width();
    int height = image->height();

    // Bounds check
    if (startX < 0 || startX >= width || startY < 0 || startY >= height) {
        return region;
    }

    bool startTransparent = isTransparent(image, startX, startY, palette);
    SHADE_LOG("fillBoundedArea: startTransparent=%d", startTransparent);

    if (startTransparent) {
        // Starting on transparent pixel - fill interior bounded by outline
        // First check if this transparent area is truly bounded (doesn't touch edges)
        std::queue<gfx::Point> queue;
        PointSet visited;
        PointSet interiorPixels;   // Transparent pixels inside the shape
        PointSet boundaryPixels;   // Non-transparent boundary pixels
        bool touchesEdge = false;

        queue.push(gfx::Point(startX, startY));

        while (!queue.empty()) {
            gfx::Point current = queue.front();
            queue.pop();

            if (visited.find(current) != visited.end()) {
                continue;
            }

            // Check if we've escaped to the image edge
            if (current.x < 0 || current.x >= width ||
                current.y < 0 || current.y >= height) {
                touchesEdge = true;
                continue;
            }

            visited.insert(current);

            bool currentTransparent = isTransparent(image, current.x, current.y, palette);

            if (!currentTransparent) {
                // Hit boundary - record it but don't expand
                boundaryPixels.insert(current);
                continue;
            }

            // Transparent pixel - add to interior
            interiorPixels.insert(current);

            // Expand to neighbors
            queue.push(gfx::Point(current.x + 1, current.y));
            queue.push(gfx::Point(current.x - 1, current.y));
            queue.push(gfx::Point(current.x, current.y + 1));
            queue.push(gfx::Point(current.x, current.y - 1));
        }

        // If the transparent area touches the edge, it's not bounded
        if (touchesEdge) {
            SHADE_LOG("fillBoundedArea: touches edge - not bounded, returning empty");
            return region;  // Return empty - not a closed shape
        }

        SHADE_LOG("fillBoundedArea: bounded! interior=%zu boundary=%zu",
                  interiorPixels.size(), boundaryPixels.size());

        // Return interior + boundary
        for (const auto& p : interiorPixels) {
            region.push_back(p);
        }
        for (const auto& p : boundaryPixels) {
            region.push_back(p);
        }
    }
    else {
        SHADE_LOG("fillBoundedArea: starting on non-transparent pixel");
        // Starting on non-transparent pixel (outline)
        // Get all connected non-transparent pixels (the outline/shape)
        std::queue<gfx::Point> queue;
        PointSet visited;
        PointSet shapePixels;

        queue.push(gfx::Point(startX, startY));

        while (!queue.empty()) {
            gfx::Point current = queue.front();
            queue.pop();

            if (visited.find(current) != visited.end()) {
                continue;
            }

            if (current.x < 0 || current.x >= width ||
                current.y < 0 || current.y >= height) {
                continue;
            }

            visited.insert(current);

            if (isTransparent(image, current.x, current.y, palette)) {
                continue;  // Don't expand through transparent
            }

            shapePixels.insert(current);

            queue.push(gfx::Point(current.x + 1, current.y));
            queue.push(gfx::Point(current.x - 1, current.y));
            queue.push(gfx::Point(current.x, current.y + 1));
            queue.push(gfx::Point(current.x, current.y - 1));
        }

        // Now find interior transparent regions bounded by this shape
        // Check each transparent pixel adjacent to the shape
        PointSet checkedInteriors;
        PointSet allInteriorPixels;

        for (const auto& sp : shapePixels) {
            // Check 4 neighbors for transparent starting points
            gfx::Point neighbors[4] = {
                gfx::Point(sp.x + 1, sp.y),
                gfx::Point(sp.x - 1, sp.y),
                gfx::Point(sp.x, sp.y + 1),
                gfx::Point(sp.x, sp.y - 1)
            };

            for (const auto& neighbor : neighbors) {
                if (neighbor.x < 0 || neighbor.x >= width ||
                    neighbor.y < 0 || neighbor.y >= height) {
                    continue;
                }

                if (checkedInteriors.find(neighbor) != checkedInteriors.end()) {
                    continue;
                }

                if (!isTransparent(image, neighbor.x, neighbor.y, palette)) {
                    continue;
                }

                // Flood fill this transparent region to see if it's bounded
                std::queue<gfx::Point> interiorQueue;
                PointSet interiorVisited;
                PointSet candidateInterior;
                bool bounded = true;

                interiorQueue.push(neighbor);

                while (!interiorQueue.empty()) {
                    gfx::Point curr = interiorQueue.front();
                    interiorQueue.pop();

                    if (interiorVisited.find(curr) != interiorVisited.end()) {
                        continue;
                    }

                    if (curr.x < 0 || curr.x >= width ||
                        curr.y < 0 || curr.y >= height) {
                        bounded = false;
                        continue;
                    }

                    interiorVisited.insert(curr);
                    checkedInteriors.insert(curr);

                    if (!isTransparent(image, curr.x, curr.y, palette)) {
                        continue;  // Hit shape boundary
                    }

                    candidateInterior.insert(curr);

                    interiorQueue.push(gfx::Point(curr.x + 1, curr.y));
                    interiorQueue.push(gfx::Point(curr.x - 1, curr.y));
                    interiorQueue.push(gfx::Point(curr.x, curr.y + 1));
                    interiorQueue.push(gfx::Point(curr.x, curr.y - 1));
                }

                if (bounded) {
                    for (const auto& ip : candidateInterior) {
                        allInteriorPixels.insert(ip);
                    }
                }
            }
        }

        // Return shape pixels + bounded interior
        for (const auto& p : shapePixels) {
            region.push_back(p);
        }
        for (const auto& p : allInteriorPixels) {
            region.push_back(p);
        }
    }

    return region;
}

bool FloodFill::isTransparent(const doc::Image* image, int x, int y, const doc::Palette* palette)
{
    doc::color_t pixelColor = doc::get_pixel(image, x, y);
    doc::PixelFormat format = image->pixelFormat();

    switch (format) {
        case doc::IMAGE_RGB:
            return doc::rgba_geta(pixelColor) == 0;

        case doc::IMAGE_INDEXED:
            if (palette) {
                doc::color_t rgb = getIndexedColor(palette, pixelColor);
                return doc::rgba_geta(rgb) == 0;
            }
            // Index 0 is typically transparent in indexed images
            return pixelColor == 0;

        case doc::IMAGE_GRAYSCALE:
            return doc::graya_geta(pixelColor) == 0;

        default:
            return false;
    }
}

doc::color_t FloodFill::getIndexedColor(const doc::Palette* palette, doc::color_t index)
{
    if (!palette || index >= palette->size()) {
        return doc::rgba(0, 0, 0, 0);
    }
    return palette->getEntry(index);
}

double FloodFill::colorDistance(doc::color_t c1, doc::color_t c2)
{
    int r1 = doc::rgba_getr(c1);
    int g1 = doc::rgba_getg(c1);
    int b1 = doc::rgba_getb(c1);

    int r2 = doc::rgba_getr(c2);
    int g2 = doc::rgba_getg(c2);
    int b2 = doc::rgba_getb(c2);

    int dr = r1 - r2;
    int dg = g1 - g2;
    int db = b1 - b2;

    return std::sqrt(dr * dr + dg * dg + db * db);
}

bool FloodFill::colorMatches(doc::color_t c1, doc::color_t c2, int tolerance)
{
    if (tolerance == 0) {
        // Exact match (ignore alpha for now)
        return (doc::rgba_getr(c1) == doc::rgba_getr(c2) &&
                doc::rgba_getg(c1) == doc::rgba_getg(c2) &&
                doc::rgba_getb(c1) == doc::rgba_getb(c2));
    }

    // Calculate Euclidean distance in RGB space
    // Max possible distance is sqrt(255^2 * 3) ≈ 441
    double dist = colorDistance(c1, c2);

    // Tolerance is 0-255, scale it to compare with RGB distance
    return dist <= tolerance * 1.732;  // sqrt(3) scaling factor
}

} // namespace tools
} // namespace app
