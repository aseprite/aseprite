// pen_path.cpp
// Implementation of PenPath class

#include "pen_path.h"
#include <algorithm>
#include <stdexcept>
#include <climits>

namespace app {
namespace tools {

//------------------------------------------------------------------------------
// Constructors
//------------------------------------------------------------------------------

PenPath::PenPath()
    : m_closed(false)
{
}

PenPath::~PenPath()
{
}

//------------------------------------------------------------------------------
// Anchor management
//------------------------------------------------------------------------------

void PenPath::addAnchor(const PenAnchor& anchor)
{
    m_anchors.push_back(anchor);
}

void PenPath::insertAnchor(int index, const PenAnchor& anchor)
{
    if (index < 0 || index > static_cast<int>(m_anchors.size())) {
        throw std::out_of_range("Anchor index out of range");
    }
    m_anchors.insert(m_anchors.begin() + index, anchor);
}

void PenPath::removeAnchor(int index)
{
    if (index < 0 || index >= static_cast<int>(m_anchors.size())) {
        throw std::out_of_range("Anchor index out of range");
    }
    m_anchors.erase(m_anchors.begin() + index);
    
    // If we removed enough points, can't be closed anymore
    if (m_anchors.size() < 3) {
        m_closed = false;
    }
}

void PenPath::removeLastAnchor()
{
    if (!m_anchors.empty()) {
        m_anchors.pop_back();
        
        if (m_anchors.size() < 3) {
            m_closed = false;
        }
    }
}

void PenPath::updateAnchor(int index, const PenAnchor& anchor)
{
    if (index < 0 || index >= static_cast<int>(m_anchors.size())) {
        throw std::out_of_range("Anchor index out of range");
    }
    m_anchors[index] = anchor;
}

void PenPath::moveAnchor(int index, const gfx::Point& newPos)
{
    if (index < 0 || index >= static_cast<int>(m_anchors.size())) {
        throw std::out_of_range("Anchor index out of range");
    }
    
    // Calculate delta
    gfx::Point delta(
        newPos.x - m_anchors[index].position.x,
        newPos.y - m_anchors[index].position.y
    );
    
    // Move position (handles stay relative, so they move with it)
    m_anchors[index].position = newPos;
}

//------------------------------------------------------------------------------
// Accessors
//------------------------------------------------------------------------------

const PenAnchor& PenPath::getAnchor(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_anchors.size())) {
        throw std::out_of_range("Anchor index out of range");
    }
    return m_anchors[index];
}

PenAnchor& PenPath::getAnchor(int index)
{
    if (index < 0 || index >= static_cast<int>(m_anchors.size())) {
        throw std::out_of_range("Anchor index out of range");
    }
    return m_anchors[index];
}

//------------------------------------------------------------------------------
// Geometry
//------------------------------------------------------------------------------

gfx::Rect PenPath::getBounds() const
{
    if (m_anchors.empty()) {
        return gfx::Rect(0, 0, 0, 0);
    }
    
    int minX = INT_MAX, minY = INT_MAX;
    int maxX = INT_MIN, maxY = INT_MIN;
    
    for (const auto& anchor : m_anchors) {
        minX = std::min(minX, anchor.position.x);
        minY = std::min(minY, anchor.position.y);
        maxX = std::max(maxX, anchor.position.x);
        maxY = std::max(maxY, anchor.position.y);
    }
    
    return gfx::Rect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

gfx::Rect PenPath::getBoundsWithHandles() const
{
    if (m_anchors.empty()) {
        return gfx::Rect(0, 0, 0, 0);
    }
    
    int minX = INT_MAX, minY = INT_MAX;
    int maxX = INT_MIN, maxY = INT_MIN;
    
    for (const auto& anchor : m_anchors) {
        // Check position
        minX = std::min(minX, anchor.position.x);
        minY = std::min(minY, anchor.position.y);
        maxX = std::max(maxX, anchor.position.x);
        maxY = std::max(maxY, anchor.position.y);
        
        // Check handle in (absolute)
        gfx::Point hIn = anchor.getHandleInAbsolute();
        minX = std::min(minX, hIn.x);
        minY = std::min(minY, hIn.y);
        maxX = std::max(maxX, hIn.x);
        maxY = std::max(maxY, hIn.y);
        
        // Check handle out (absolute)
        gfx::Point hOut = anchor.getHandleOutAbsolute();
        minX = std::min(minX, hOut.x);
        minY = std::min(minY, hOut.y);
        maxX = std::max(maxX, hOut.x);
        maxY = std::max(maxY, hOut.y);
    }
    
    return gfx::Rect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

//------------------------------------------------------------------------------
// Operations
//------------------------------------------------------------------------------

void PenPath::clear()
{
    m_anchors.clear();
    m_closed = false;
}

void PenPath::reverse()
{
    if (m_anchors.size() < 2) {
        return;
    }
    
    // Reverse anchor order
    std::reverse(m_anchors.begin(), m_anchors.end());
    
    // Swap handles for each anchor (in becomes out, out becomes in)
    for (auto& anchor : m_anchors) {
        std::swap(anchor.handleIn, anchor.handleOut);
    }
}

void PenPath::translate(const gfx::Point& delta)
{
    for (auto& anchor : m_anchors) {
        anchor.position.x += delta.x;
        anchor.position.y += delta.y;
        // Note: Relative handles don't need to change
    }
}

} // namespace tools
} // namespace app
