// pen_anchor.h
// Anchor point for Pixel Pen Tool paths
//
// An anchor represents a point on the path with optional
// Bezier curve handles for smooth curves.

#ifndef APP_TOOLS_PIXEL_PEN_PEN_ANCHOR_H
#define APP_TOOLS_PIXEL_PEN_PEN_ANCHOR_H

#include <cmath>
#include "gfx/point.h"

namespace app {
namespace tools {

//------------------------------------------------------------------------------
// AnchorType - Determines how handles behave
//------------------------------------------------------------------------------

enum class AnchorType {
    Corner,         // Sharp corner - handles are independent
    Smooth,         // Smooth curve - handles are collinear (opposite directions)
    Symmetric       // Symmetric - handles mirror each other (same length + opposite)
};

//------------------------------------------------------------------------------
// PenAnchor - A point on the path
//------------------------------------------------------------------------------

struct PenAnchor {
    //--------------------------------------------------------------------------
    // Data
    //--------------------------------------------------------------------------
    
    gfx::Point position;        // Anchor pixel position
    gfx::Point handleIn;        // Incoming curve handle (relative to position)
    gfx::Point handleOut;       // Outgoing curve handle (relative to position)
    AnchorType type;            // How this anchor behaves
    
    //--------------------------------------------------------------------------
    // Constructors
    //--------------------------------------------------------------------------
    
    PenAnchor() 
        : position(0, 0)
        , handleIn(0, 0)
        , handleOut(0, 0)
        , type(AnchorType::Corner) 
    {}
    
    explicit PenAnchor(const gfx::Point& pos)
        : position(pos)
        , handleIn(0, 0)
        , handleOut(0, 0)
        , type(AnchorType::Corner) 
    {}
    
    PenAnchor(const gfx::Point& pos, AnchorType t)
        : position(pos)
        , handleIn(0, 0)
        , handleOut(0, 0)
        , type(t) 
    {}
    
    PenAnchor(const gfx::Point& pos, const gfx::Point& hIn, const gfx::Point& hOut)
        : position(pos)
        , handleIn(hIn)
        , handleOut(hOut)
        , type(AnchorType::Smooth) 
    {}
    
    //--------------------------------------------------------------------------
    // Handle queries
    //--------------------------------------------------------------------------
    
    /// Check if this anchor has any curve handles defined
    bool hasCurve() const {
        return handleIn.x != 0 || handleIn.y != 0 || 
               handleOut.x != 0 || handleOut.y != 0;
    }
    
    /// Check if incoming handle is defined
    bool hasHandleIn() const {
        return handleIn.x != 0 || handleIn.y != 0;
    }
    
    /// Check if outgoing handle is defined
    bool hasHandleOut() const {
        return handleOut.x != 0 || handleOut.y != 0;
    }
    
    //--------------------------------------------------------------------------
    // Absolute handle positions
    //--------------------------------------------------------------------------
    
    /// Get incoming handle position in absolute coordinates
    gfx::Point getHandleInAbsolute() const {
        return gfx::Point(position.x + handleIn.x, position.y + handleIn.y);
    }
    
    /// Get outgoing handle position in absolute coordinates
    gfx::Point getHandleOutAbsolute() const {
        return gfx::Point(position.x + handleOut.x, position.y + handleOut.y);
    }
    
    //--------------------------------------------------------------------------
    // Handle manipulation
    //--------------------------------------------------------------------------
    
    /// Set incoming handle from absolute position
    void setHandleInAbsolute(const gfx::Point& absPos) {
        handleIn = gfx::Point(absPos.x - position.x, absPos.y - position.y);
        enforceType();
    }
    
    /// Set outgoing handle from absolute position
    void setHandleOutAbsolute(const gfx::Point& absPos) {
        handleOut = gfx::Point(absPos.x - position.x, absPos.y - position.y);
        enforceType();
    }
    
    /// Clear all handles (make corner)
    void clearHandles() {
        handleIn = gfx::Point(0, 0);
        handleOut = gfx::Point(0, 0);
        type = AnchorType::Corner;
    }
    
    //--------------------------------------------------------------------------
    // Type conversion
    //--------------------------------------------------------------------------
    
    /// Toggle between Corner and Smooth
    void toggleSmooth() {
        if (type == AnchorType::Corner) {
            type = AnchorType::Smooth;
            // Make handles collinear if both exist
            if (hasHandleIn() && hasHandleOut()) {
                enforceType();
            }
        } else {
            type = AnchorType::Corner;
        }
    }

    /// Cycle through all anchor types: Corner -> Smooth -> Symmetric -> Corner
    void cycleType() {
        switch (type) {
            case AnchorType::Corner:
                type = AnchorType::Smooth;
                break;
            case AnchorType::Smooth:
                type = AnchorType::Symmetric;
                break;
            case AnchorType::Symmetric:
                type = AnchorType::Corner;
                break;
        }
        enforceType();
    }

    /// Get type name for status bar display
    const char* getTypeName() const {
        switch (type) {
            case AnchorType::Corner: return "Corner";
            case AnchorType::Smooth: return "Smooth";
            case AnchorType::Symmetric: return "Symmetric";
        }
        return "Unknown";
    }
    
private:
    /// Enforce handle constraints based on type
    void enforceType() {
        if (type == AnchorType::Symmetric) {
            // Mirror handles exactly
            handleIn = gfx::Point(-handleOut.x, -handleOut.y);
        } 
        else if (type == AnchorType::Smooth) {
            // Make handles collinear (opposite directions, can be different lengths)
            if (hasHandleOut() && hasHandleIn()) {
                // Keep handleOut length, adjust handleIn direction
                double outLen = std::sqrt(handleOut.x * handleOut.x + handleOut.y * handleOut.y);
                double inLen = std::sqrt(handleIn.x * handleIn.x + handleIn.y * handleIn.y);
                
                if (outLen > 0 && inLen > 0) {
                    double scale = inLen / outLen;
                    handleIn = gfx::Point(
                        static_cast<int>(-handleOut.x * scale),
                        static_cast<int>(-handleOut.y * scale)
                    );
                }
            }
        }
        // Corner type: no constraints, handles are independent
    }
};

//------------------------------------------------------------------------------
// Comparison operators
//------------------------------------------------------------------------------

inline bool operator==(const PenAnchor& a, const PenAnchor& b) {
    return a.position == b.position &&
           a.handleIn == b.handleIn &&
           a.handleOut == b.handleOut &&
           a.type == b.type;
}

inline bool operator!=(const PenAnchor& a, const PenAnchor& b) {
    return !(a == b);
}

} // namespace tools
} // namespace app

#endif // APP_TOOLS_PIXEL_PEN_PEN_ANCHOR_H
