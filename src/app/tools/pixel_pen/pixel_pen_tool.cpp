// Aseprite
// Copyright (C) 2026  Custom Build
//
// Pixel Pen Tool - Main tool implementation

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pixel_pen_tool.h"
#include "doc/image.h"
#include "doc/primitives.h"

namespace app {
namespace tools {

//------------------------------------------------------------------------------
// Constructor / Destructor
//------------------------------------------------------------------------------

PixelPenTool::PixelPenTool()
    : m_selectedAnchor(-1)
    , m_isDrawing(false)
    , m_draggingHandle(false)
    , m_dragHandleType(0)
{
}

PixelPenTool::~PixelPenTool()
{
}

//------------------------------------------------------------------------------
// Selection
//------------------------------------------------------------------------------

void PixelPenTool::selectAnchor(int index)
{
    if (index >= 0 && index < m_path.anchorCount()) {
        m_selectedAnchor = index;
    }
    else {
        m_selectedAnchor = -1;
    }
}

//------------------------------------------------------------------------------
// Preview generation
//------------------------------------------------------------------------------

std::vector<gfx::Point> PixelPenTool::generatePreview()
{
    if (m_path.isEmpty()) {
        return std::vector<gfx::Point>();
    }

    return m_calculator.calculatePath(m_path.anchors(), m_path.isClosed());
}

std::vector<gfx::Point> PixelPenTool::generatePreviewWithTemp(const gfx::Point& tempPoint)
{
    if (m_path.isEmpty()) {
        // Just return the temp point
        return std::vector<gfx::Point>{tempPoint};
    }

    // Create a temporary path with the extra point
    std::vector<PenAnchor> tempAnchors = m_path.anchors();
    tempAnchors.push_back(PenAnchor(tempPoint));

    return m_calculator.calculatePath(tempAnchors, false);
}

//------------------------------------------------------------------------------
// Commit to document
//------------------------------------------------------------------------------

void PixelPenTool::commit(doc::Image* image, doc::color_t color)
{
    if (!image || m_path.isEmpty()) {
        return;
    }

    // Save the path before committing (for restore feature)
    m_lastCommittedPath = m_path;

    // Generate final pixels
    std::vector<gfx::Point> pixels = generatePreview();

    // Draw each pixel
    for (const auto& pt : pixels) {
        // Bounds check
        if (pt.x >= 0 && pt.x < image->width() &&
            pt.y >= 0 && pt.y < image->height()) {
            doc::put_pixel(image, pt.x, pt.y, color);
        }
    }
}

//------------------------------------------------------------------------------
// Reset
//------------------------------------------------------------------------------

void PixelPenTool::reset()
{
    m_path.clear();
    m_selectedAnchor = -1;
    m_isDrawing = false;
    m_draggingHandle = false;
    m_dragHandleType = 0;
}

//------------------------------------------------------------------------------
// Handle manipulation
//------------------------------------------------------------------------------

bool PixelPenTool::createDefaultHandles()
{
    if (m_selectedAnchor < 0 || m_selectedAnchor >= m_path.anchorCount()) {
        return false;
    }

    PenAnchor& anchor = m_path.getAnchor(m_selectedAnchor);
    int count = m_path.anchorCount();

    // Default handle length (in pixels)
    const int defaultHandleLen = 10;

    // Calculate handleIn based on previous anchor
    if (m_selectedAnchor > 0 || m_path.isClosed()) {
        int prevIdx = (m_selectedAnchor > 0) ? m_selectedAnchor - 1 : count - 1;
        const PenAnchor& prev = m_path.getAnchor(prevIdx);

        // Direction from anchor toward previous
        int dx = prev.position.x - anchor.position.x;
        int dy = prev.position.y - anchor.position.y;
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 0) {
            // Handle length is 1/3 of distance or default, whichever is smaller
            double handleLen = std::min(dist / 3.0, static_cast<double>(defaultHandleLen));
            anchor.handleIn.x = static_cast<int>(dx / dist * handleLen);
            anchor.handleIn.y = static_cast<int>(dy / dist * handleLen);
        }
    }

    // Calculate handleOut based on next anchor
    if (m_selectedAnchor < count - 1 || m_path.isClosed()) {
        int nextIdx = (m_selectedAnchor < count - 1) ? m_selectedAnchor + 1 : 0;
        const PenAnchor& next = m_path.getAnchor(nextIdx);

        // Direction from anchor toward next
        int dx = next.position.x - anchor.position.x;
        int dy = next.position.y - anchor.position.y;
        double dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 0) {
            // Handle length is 1/3 of distance or default, whichever is smaller
            double handleLen = std::min(dist / 3.0, static_cast<double>(defaultHandleLen));
            anchor.handleOut.x = static_cast<int>(dx / dist * handleLen);
            anchor.handleOut.y = static_cast<int>(dy / dist * handleLen);
        }
    }

    // Change type to Smooth if both handles exist
    if (anchor.hasHandleIn() && anchor.hasHandleOut()) {
        anchor.type = AnchorType::Smooth;
    }
    else if (anchor.hasHandleIn() || anchor.hasHandleOut()) {
        anchor.type = AnchorType::Corner;
    }

    return anchor.hasCurve();
}

void PixelPenTool::resetHandles()
{
    if (m_selectedAnchor < 0 || m_selectedAnchor >= m_path.anchorCount()) {
        return;
    }

    PenAnchor& anchor = m_path.getAnchor(m_selectedAnchor);

    // Clear existing handles
    anchor.handleIn = gfx::Point(0, 0);
    anchor.handleOut = gfx::Point(0, 0);

    // Create new default handles
    createDefaultHandles();
}

//------------------------------------------------------------------------------
// Last path restore
//------------------------------------------------------------------------------

void PixelPenTool::restoreLastPath()
{
    if (!m_lastCommittedPath.isEmpty()) {
        m_path = m_lastCommittedPath;
        m_selectedAnchor = -1;
        m_isDrawing = true;
        m_draggingHandle = false;
        m_dragHandleType = 0;
    }
}

} // namespace tools
} // namespace app
