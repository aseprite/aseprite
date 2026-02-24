// pixel_line_calculator.h
// Core rules engine for pixel-perfect line generation
//
// Implements pixel art stroke rules:
// - Consistent 1px thickness
// - Balanced run progression
// - Proper curve handling
// - Anchor point preservation

#ifndef APP_TOOLS_PIXEL_PEN_PIXEL_LINE_CALCULATOR_H
#define APP_TOOLS_PIXEL_PEN_PIXEL_LINE_CALCULATOR_H

#include "gfx/point.h"
#include <vector>

namespace app {
namespace tools {

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------

struct PenAnchor;

//------------------------------------------------------------------------------
// PixelRun - Represents a sequence of pixels in one direction
//------------------------------------------------------------------------------

struct PixelRun {
    int length;             // Number of pixels in this run
    gfx::Point direction;   // Direction of travel
    
    PixelRun() : length(0), direction(0, 0) {}
    PixelRun(int len, const gfx::Point& dir) : length(len), direction(dir) {}
};

//------------------------------------------------------------------------------
// PixelLineCalculator - The rules engine
//------------------------------------------------------------------------------

class PixelLineCalculator {
public:
    PixelLineCalculator();
    
    //--------------------------------------------------------------------------
    // Main API
    //--------------------------------------------------------------------------
    
    /// Generate pixel-perfect line between two anchors
    /// Applies all pixel art rules automatically
    std::vector<gfx::Point> calculateSegment(
        const PenAnchor& from,
        const PenAnchor& to
    );
    
    /// Generate complete path from all anchors
    /// @param anchors List of anchor points
    /// @param closed If true, connects last anchor back to first
    std::vector<gfx::Point> calculatePath(
        const std::vector<PenAnchor>& anchors,
        bool closed
    );
    
    //--------------------------------------------------------------------------
    // Analysis
    //--------------------------------------------------------------------------
    
    /// Analyze a pixel line and return its run structure
    std::vector<PixelRun> analyzeRuns(const std::vector<gfx::Point>& pixels);
    
    /// Check if runs follow balanced progression rules
    /// Returns true if no values are skipped (e.g., 1→2→3 is good, 1→3 is bad)
    bool isBalanced(const std::vector<PixelRun>& runs);
    
    //--------------------------------------------------------------------------
    // Configuration
    //--------------------------------------------------------------------------
    
    /// Enable/disable automatic run balancing
    void setAutoBalance(bool enabled) { m_autoBalance = enabled; }
    bool autoBalance() const { return m_autoBalance; }
    
    /// Maximum allowed run length (for style consistency)
    void setMaxRunLength(int max) { m_maxRunLength = max; }
    int maxRunLength() const { return m_maxRunLength; }
    
private:
    //--------------------------------------------------------------------------
    // Internal state
    //--------------------------------------------------------------------------
    
    bool m_autoBalance;
    int m_maxRunLength;
    
    //--------------------------------------------------------------------------
    // Line generation algorithms
    //--------------------------------------------------------------------------
    
    /// Generate a straight line with consistent pixel ratios
    std::vector<gfx::Point> calculateStraightLine(
        const gfx::Point& from,
        const gfx::Point& to
    );
    
    /// Generate a cubic Bezier curve
    /// @param p0 Start point
    /// @param p1 Control point 1 (handle out of p0)
    /// @param p2 Control point 2 (handle in of p3)
    /// @param p3 End point
    std::vector<gfx::Point> calculateBezierCurve(
        const gfx::Point& p0,
        const gfx::Point& p1,
        const gfx::Point& p2,
        const gfx::Point& p3
    );
    
    //--------------------------------------------------------------------------
    // Post-processing
    //--------------------------------------------------------------------------
    
    /// Balance runs to ensure smooth progression (no skipped values)
    void balanceRuns(std::vector<gfx::Point>& pixels);
    
    /// Ensure line maintains consistent thickness (usually 1px)
    void enforceThickness(std::vector<gfx::Point>& pixels);
    
    /// Remove consecutive duplicate pixels
    void removeDuplicates(std::vector<gfx::Point>& pixels);
};

} // namespace tools
} // namespace app

#endif // APP_TOOLS_PIXEL_PEN_PIXEL_LINE_CALCULATOR_H
