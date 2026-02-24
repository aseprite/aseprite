// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade State - Editor state for the Auto-Shade Tool
//
// This state handles mouse/keyboard input for selecting regions
// and applying automatic shading based on light direction.

#ifndef APP_UI_EDITOR_AUTO_SHADE_STATE_H_INCLUDED
#define APP_UI_EDITOR_AUTO_SHADE_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/state_with_wheel_behavior.h"
#include "app/tools/auto_shade/auto_shade_tool.h"
#include "gfx/point.h"

#include <unordered_map>

namespace app {

class Editor;
class EditorDecorator;

// Editor state for the Auto-Shade Tool
// Handles region selection and shading preview/application
class AutoShadeState : public StateWithWheelBehavior {
public:
    AutoShadeState(Editor* editor);
    virtual ~AutoShadeState();

    //--------------------------------------------------------------------------
    // State interface
    //--------------------------------------------------------------------------

    virtual void onEnterState(Editor* editor) override;
    virtual LeaveAction onLeaveState(Editor* editor, EditorState* newState) override;
    virtual void onBeforePopState(Editor* editor) override;
    virtual void onActiveToolChange(Editor* editor, tools::Tool* tool) override;

    //--------------------------------------------------------------------------
    // Mouse events
    //--------------------------------------------------------------------------

    virtual bool onMouseDown(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseMove(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onMouseUp(Editor* editor, ui::MouseMessage* msg) override;
    virtual bool onDoubleClick(Editor* editor, ui::MouseMessage* msg) override;

    //--------------------------------------------------------------------------
    // Keyboard events
    //--------------------------------------------------------------------------

    virtual bool onKeyDown(Editor* editor, ui::KeyMessage* msg) override;
    virtual bool onKeyUp(Editor* editor, ui::KeyMessage* msg) override;

    //--------------------------------------------------------------------------
    // Cursor
    //--------------------------------------------------------------------------

    virtual bool onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos) override;

    //--------------------------------------------------------------------------
    // Status bar
    //--------------------------------------------------------------------------

    virtual bool onUpdateStatusBar(Editor* editor) override;

    //--------------------------------------------------------------------------
    // Drawing
    //--------------------------------------------------------------------------

    virtual void onExposeSpritePixels(const gfx::Region& rgn) override;

    // This state doesn't use the standard brush preview
    virtual bool requireBrushPreview() override { return false; }

    // Allow layer edges
    virtual bool allowLayerEdges() override { return true; }

    //--------------------------------------------------------------------------
    // Configuration access (for context bar)
    //--------------------------------------------------------------------------

    tools::ShadeConfig& config() { return m_tool.config(); }
    const tools::ShadeConfig& config() const { return m_tool.config(); }

    // Update preview when config changes
    void updatePreview(Editor* editor);

    // Check if there's a preview to apply
    bool hasPreview() const { return m_hasPreview; }

    // Apply current preview
    void applyShading(Editor* editor);

    // Cancel and clear preview
    void cancelPreview(Editor* editor);

private:
    //--------------------------------------------------------------------------
    // Actions
    //--------------------------------------------------------------------------

    // Analyze region at position and generate preview
    void analyzeRegion(Editor* editor, const gfx::Point& spritePos);

    // Generate shaded preview using original colors
    void generateShadedPreview();

    //--------------------------------------------------------------------------
    // Drawing helpers
    //--------------------------------------------------------------------------

    // Invalidate editor region for redraw
    void invalidateEditor(Editor* editor);

    //--------------------------------------------------------------------------
    // Light handle helpers
    //--------------------------------------------------------------------------

    // Get light handle position in sprite coordinates
    gfx::Point getLightHandlePosition() const;

    // Check if a screen position hits the light handle
    bool hitTestLightHandle(Editor* editor, const gfx::Point& screenPos) const;

    //--------------------------------------------------------------------------
    // Highlight point helpers (incidence point - where light hits the shape)
    //--------------------------------------------------------------------------

    // Get highlight point position in sprite coordinates
    gfx::Point getHighlightPosition() const;

    // Check if a screen position hits the highlight point
    bool hitTestHighlightPoint(Editor* editor, const gfx::Point& screenPos) const;

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------

    tools::AutoShadeTool m_tool;
    gfx::Point m_lastMousePos;
    gfx::Point m_clickPos;          // Position where region was selected
    bool m_hasPreview;              // True if preview is currently active
    bool m_mouseDown;
    bool m_draggingLightHandle;     // True when dragging the light direction handle
    bool m_draggingHighlightPoint;  // True when dragging the highlight/incidence point
    double m_highlightDistanceRatio; // 0.0 = center, 1.0 = edge (where highlight appears)
    double m_highlightAngle;        // Angle in degrees (independent of light angle)

    // Preview color map (pixel position -> shaded color)
    std::unordered_map<gfx::Point, doc::color_t, tools::PointHash> m_previewColors;

    // Original colors from the image (pixel position -> original color)
    std::unordered_map<gfx::Point, doc::color_t, tools::PointHash> m_originalColors;

    // Decorator for rendering
    class Decorator;
    Decorator* m_decorator;
};

} // namespace app

#endif // APP_UI_EDITOR_AUTO_SHADE_STATE_H_INCLUDED
