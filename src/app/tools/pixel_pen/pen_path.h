// pen_path.h
// Path container for Pixel Pen Tool
//
// A PenPath holds a collection of anchors that define
// a vector path to be converted to pixels.

#ifndef APP_TOOLS_PIXEL_PEN_PEN_PATH_H
#define APP_TOOLS_PIXEL_PEN_PEN_PATH_H

#include "pen_anchor.h"
#include "gfx/rect.h"
#include <vector>

namespace app {
namespace tools {

//------------------------------------------------------------------------------
// PenPath - Collection of anchors forming a path
//------------------------------------------------------------------------------

class PenPath {
public:
    //--------------------------------------------------------------------------
    // Constructors
    //--------------------------------------------------------------------------
    
    PenPath();
    ~PenPath();
    
    //--------------------------------------------------------------------------
    // Anchor management
    //--------------------------------------------------------------------------
    
    /// Add anchor at end of path
    void addAnchor(const PenAnchor& anchor);
    
    /// Insert anchor at specific index
    void insertAnchor(int index, const PenAnchor& anchor);
    
    /// Remove anchor at index
    void removeAnchor(int index);
    
    /// Remove last anchor (undo last point)
    void removeLastAnchor();
    
    /// Update anchor at index
    void updateAnchor(int index, const PenAnchor& anchor);
    
    /// Move anchor position (updates handles relatively)
    void moveAnchor(int index, const gfx::Point& newPos);
    
    //--------------------------------------------------------------------------
    // Accessors
    //--------------------------------------------------------------------------
    
    /// Number of anchors in path
    int anchorCount() const { return static_cast<int>(m_anchors.size()); }
    
    /// Check if path is empty
    bool isEmpty() const { return m_anchors.empty(); }
    
    /// Get anchor at index (const)
    const PenAnchor& getAnchor(int index) const;
    
    /// Get anchor at index (mutable)
    PenAnchor& getAnchor(int index);
    
    /// Get first anchor
    const PenAnchor& firstAnchor() const { return m_anchors.front(); }
    
    /// Get last anchor
    const PenAnchor& lastAnchor() const { return m_anchors.back(); }
    
    /// Get all anchors
    const std::vector<PenAnchor>& anchors() const { return m_anchors; }
    
    //--------------------------------------------------------------------------
    // Path state
    //--------------------------------------------------------------------------
    
    /// Is this a closed path?
    bool isClosed() const { return m_closed; }
    
    /// Set closed state
    void setClosed(bool closed) { m_closed = closed; }
    
    /// Toggle closed state
    void toggleClosed() { m_closed = !m_closed; }
    
    //--------------------------------------------------------------------------
    // Geometry
    //--------------------------------------------------------------------------
    
    /// Get bounding rectangle of all anchors
    gfx::Rect getBounds() const;
    
    /// Get bounding rectangle including handles
    gfx::Rect getBoundsWithHandles() const;
    
    //--------------------------------------------------------------------------
    // Operations
    //--------------------------------------------------------------------------
    
    /// Clear all anchors
    void clear();
    
    /// Reverse path direction
    void reverse();
    
    /// Translate entire path
    void translate(const gfx::Point& delta);
    
    //--------------------------------------------------------------------------
    // Validation
    //--------------------------------------------------------------------------
    
    /// Check if path has enough points to be valid
    bool isValid() const { return m_anchors.size() >= 2; }
    
    /// Check if path can be closed (needs at least 3 points)
    bool canClose() const { return m_anchors.size() >= 3; }
    
private:
    std::vector<PenAnchor> m_anchors;
    bool m_closed;
};

} // namespace tools
} // namespace app

#endif // APP_TOOLS_PIXEL_PEN_PEN_PATH_H
