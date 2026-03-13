// Aseprite
// Copyright (C) 2026  Custom Build
//
// Pixel Pen Tool - Main tool class
//
// This is the main wrapper for the pixel pen tool that holds
// the path, calculator, and tool state. It's used by PixelPenState
// in the editor.

#ifndef APP_TOOLS_PIXEL_PEN_PIXEL_PEN_TOOL_H
#define APP_TOOLS_PIXEL_PEN_PIXEL_PEN_TOOL_H

#include "pen_path.h"
#include "pixel_line_calculator.h"
#include "doc/color.h"

namespace doc {
class Image;
}

namespace app {
namespace tools {

//------------------------------------------------------------------------------
// PixelPenTool - Main tool class
//------------------------------------------------------------------------------

class PixelPenTool {
public:
    PixelPenTool();
    ~PixelPenTool();

    //--------------------------------------------------------------------------
    // Path access
    //--------------------------------------------------------------------------

    /// Get the current path (mutable)
    PenPath& path() { return m_path; }

    /// Get the current path (const)
    const PenPath& path() const { return m_path; }

    //--------------------------------------------------------------------------
    // Calculator access
    //--------------------------------------------------------------------------

    /// Get the pixel line calculator (mutable)
    PixelLineCalculator& calculator() { return m_calculator; }

    /// Get the pixel line calculator (const)
    const PixelLineCalculator& calculator() const { return m_calculator; }

    //--------------------------------------------------------------------------
    // Selection state
    //--------------------------------------------------------------------------

    /// Get currently selected anchor index (-1 if none)
    int selectedAnchorIndex() const { return m_selectedAnchor; }

    /// Select an anchor by index
    void selectAnchor(int index);

    /// Clear selection
    void clearSelection() { m_selectedAnchor = -1; }

    /// Check if an anchor is selected
    bool hasSelection() const { return m_selectedAnchor >= 0; }

    //--------------------------------------------------------------------------
    // Drawing state
    //--------------------------------------------------------------------------

    /// Check if currently building a path
    bool isDrawing() const { return m_isDrawing; }

    /// Set drawing state
    void setDrawing(bool drawing) { m_isDrawing = drawing; }

    /// Check if currently dragging a handle
    bool isDraggingHandle() const { return m_draggingHandle; }

    /// Set handle dragging state
    void setDraggingHandle(bool dragging) { m_draggingHandle = dragging; }

    /// Get which handle is being dragged (0=none, 1=handleIn, 2=handleOut)
    int draggingHandleType() const { return m_dragHandleType; }

    /// Set which handle is being dragged
    void setDraggingHandleType(int type) { m_dragHandleType = type; }

    //--------------------------------------------------------------------------
    // Preview generation
    //--------------------------------------------------------------------------

    /// Generate preview pixels for the current path
    std::vector<gfx::Point> generatePreview();

    /// Generate preview with a temporary point (for mouse cursor preview)
    std::vector<gfx::Point> generatePreviewWithTemp(const gfx::Point& tempPoint);

    //--------------------------------------------------------------------------
    // Commit to document
    //--------------------------------------------------------------------------

    /// Commit the path to an image
    /// @param image Target image to draw on
    /// @param color Color to use for pixels
    void commit(doc::Image* image, doc::color_t color);

    //--------------------------------------------------------------------------
    // Reset
    //--------------------------------------------------------------------------

    /// Reset all tool state
    void reset();

    //--------------------------------------------------------------------------
    // Handle manipulation
    //--------------------------------------------------------------------------

    /// Create default handles for the selected anchor based on neighbors
    /// Returns true if handles were created
    bool createDefaultHandles();

    /// Reset handles for the selected anchor to default positions
    void resetHandles();

    //--------------------------------------------------------------------------
    // Last path restore
    //--------------------------------------------------------------------------

    /// Check if there's a last committed path to restore
    bool hasLastPath() const { return !m_lastCommittedPath.isEmpty(); }

    /// Save current path as the last committed path (for restore)
    void saveLastPath() { m_lastCommittedPath = m_path; }

    /// Restore the last committed path for re-editing
    void restoreLastPath();

    /// Clear the last committed path
    void clearLastPath() { m_lastCommittedPath.clear(); }

private:
    PenPath m_path;
    PenPath m_lastCommittedPath;  // Storage for restore feature
    PixelLineCalculator m_calculator;
    int m_selectedAnchor;
    bool m_isDrawing;
    bool m_draggingHandle;
    int m_dragHandleType;
};

} // namespace tools
} // namespace app

#endif // APP_TOOLS_PIXEL_PEN_PIXEL_PEN_TOOL_H
