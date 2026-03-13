// Aseprite
// Copyright (C) 2026  Custom Build
//
// Pixel Pen State - Editor state for the Pixel Pen Tool
//
// This state handles mouse/keyboard input for placing anchor points,
// dragging bezier handles, and building vector paths that are
// converted to pixel-perfect lines.

#ifndef APP_UI_EDITOR_PIXEL_PEN_STATE_H_INCLUDED
#define APP_UI_EDITOR_PIXEL_PEN_STATE_H_INCLUDED
#pragma once

#include "app/ui/editor/state_with_wheel_behavior.h"
#include "app/tools/pixel_pen/pixel_pen_tool.h"
#include "gfx/point.h"
#include "obs/connection.h"

namespace app {

class Editor;
class EditorDecorator;

// Editor state for the Pixel Pen Tool
// Handles building vector paths with anchor points and bezier handles
class PixelPenState : public StateWithWheelBehavior {
public:
    PixelPenState(Editor* editor);
    virtual ~PixelPenState();

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
    // Path restore (for context bar button)
    //--------------------------------------------------------------------------

    // Check if there's a last path to restore
    bool hasLastPath() const { return m_tool.hasLastPath(); }

    // Restore the last committed path for re-editing
    void restoreLastPath(Editor* editor);

    //--------------------------------------------------------------------------
    // Handle manipulation (for context bar button)
    //--------------------------------------------------------------------------

    // Check if there's a selected anchor that can have handles added
    bool canAddHandles() const {
        return m_tool.hasSelection() && m_tool.path().anchorCount() > 1;
    }

    // Create default handles on the selected anchor
    void addHandlesToSelectedAnchor(Editor* editor);

private:
    //--------------------------------------------------------------------------
    // Hit testing
    //--------------------------------------------------------------------------

    // Hit test anchors, returns index or -1
    int hitTestAnchor(Editor* editor, const gfx::Point& screenPos, int threshold = 8);

    // Hit test handles, returns anchor index and sets handleType (1=in, 2=out)
    int hitTestHandle(Editor* editor, const gfx::Point& screenPos,
                      int& handleType, int threshold = 8);

    // Check if clicking near the first anchor (to close path)
    bool hitTestClosePath(Editor* editor, const gfx::Point& screenPos, int threshold = 12);

    //--------------------------------------------------------------------------
    // Actions
    //--------------------------------------------------------------------------

    // Add a new anchor at screen position
    void addAnchor(Editor* editor, const gfx::Point& screenPos);

    // Close the current path
    void closePath(Editor* editor);

    // Commit path to document
    void commitPath(Editor* editor);

    // Cancel and clear path
    void cancelPath(Editor* editor);

    // Delete selected anchor
    void deleteSelectedAnchor(Editor* editor);

    // Delete last anchor (undo last point)
    void deleteLastAnchor(Editor* editor);

    //--------------------------------------------------------------------------
    // Drawing helpers
    //--------------------------------------------------------------------------

    // Invalidate editor region for redraw
    void invalidateEditor(Editor* editor);

    //--------------------------------------------------------------------------
    // Helpers
    //--------------------------------------------------------------------------

    // Snap a target point to nearest 45-degree angle from origin
    gfx::Point snapTo45Degrees(const gfx::Point& origin, const gfx::Point& target);

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------

    tools::PixelPenTool m_tool;
    gfx::Point m_lastMousePos;
    gfx::Point m_mouseDownPos;
    bool m_mouseDown;
    bool m_creatingAnchor;    // True if we're creating a new anchor and dragging handle
    bool m_draggingAnchor;    // True when dragging to move an anchor
    int m_clickedAnchorIdx;   // Index of anchor clicked (for path closing detection)

    // Decorator for rendering
    class Decorator;
    Decorator* m_decorator;
};

} // namespace app

#endif // APP_UI_EDITOR_PIXEL_PEN_STATE_H_INCLUDED
