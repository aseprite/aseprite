// Aseprite
// Copyright (C) 2026  Custom Build
//
// Pixel Pen State - Implementation

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pixel_pen_state.h"

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/site.h"
#include "app/tools/tool_box.h"
#include "app/tx.h"
#include "app/ui/color_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_decorator.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/expand_cel_canvas.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "gfx/region.h"
#include "ui/keys.h"
#include "ui/message.h"
#include "ui/system.h"

#include "fmt/format.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace app {

using namespace ui;
using namespace tools;

//------------------------------------------------------------------------------
// Decorator - Renders the path overlay
//------------------------------------------------------------------------------

class PixelPenState::Decorator : public EditorDecorator {
public:
    Decorator(PixelPenState* state) : m_state(state) {}

    void postRenderDecorator(EditorPostRender* render) override {
        Editor* editor = render->getEditor();
        const PenPath& path = m_state->m_tool.path();

        if (path.isEmpty()) {
            return;
        }

        // Colors for rendering
        gfx::Color anchorColor = gfx::rgba(255, 255, 255, 255);
        gfx::Color anchorFillColor = gfx::rgba(0, 120, 215, 255);
        gfx::Color handleColor = gfx::rgba(100, 180, 255, 255);
        gfx::Color lineColor = gfx::rgba(0, 120, 215, 200);
        gfx::Color previewColor = gfx::rgba(255, 100, 100, 255);

        // Draw path lines (bezier control polygon)
        // Note: EditorPostRender methods expect editor/sprite coordinates,
        // they automatically convert to screen coordinates internally
        for (int i = 0; i < path.anchorCount() - 1; ++i) {
            const PenAnchor& a1 = path.getAnchor(i);
            const PenAnchor& a2 = path.getAnchor(i + 1);

            render->drawLine(lineColor, a1.position.x, a1.position.y,
                            a2.position.x, a2.position.y);
        }

        // If closed, draw line from last to first
        if (path.isClosed() && path.anchorCount() > 2) {
            const PenAnchor& first = path.firstAnchor();
            const PenAnchor& last = path.lastAnchor();
            render->drawLine(lineColor, last.position.x, last.position.y,
                            first.position.x, first.position.y);
        }

        // Get zoom scale for fixed-size UI elements
        // We want handles and anchors to be approximately the same size
        // on screen regardless of zoom, so we compute sizes in editor coords
        double zoomScale = editor->zoom().scale();
        if (zoomScale < 1.0) zoomScale = 1.0;  // Minimum scale

        // Handle marker size: aim for ~6 screen pixels
        int handleSize = std::max(1, static_cast<int>(6.0 / zoomScale));
        int handleHalf = handleSize / 2;
        if (handleHalf < 1) handleHalf = 1;

        // Draw handles and handle lines
        for (int i = 0; i < path.anchorCount(); ++i) {
            const PenAnchor& anchor = path.getAnchor(i);

            // Draw handle lines and circles (in editor coordinates)
            if (anchor.hasHandleIn()) {
                gfx::Point handleIn = anchor.getHandleInAbsolute();
                render->drawLine(handleColor, anchor.position.x, anchor.position.y,
                                handleIn.x, handleIn.y);
                render->fillRect(handleColor,
                    gfx::Rect(handleIn.x - handleHalf, handleIn.y - handleHalf,
                             handleSize, handleSize));
            }

            if (anchor.hasHandleOut()) {
                gfx::Point handleOut = anchor.getHandleOutAbsolute();
                render->drawLine(handleColor, anchor.position.x, anchor.position.y,
                                handleOut.x, handleOut.y);
                render->fillRect(handleColor,
                    gfx::Rect(handleOut.x - handleHalf, handleOut.y - handleHalf,
                             handleSize, handleSize));
            }
        }

        // Draw anchor points (on top of handles)
        // Different shapes for different types:
        // Corner = square, Smooth = circle (approx), Symmetric = diamond
        for (int i = 0; i < path.anchorCount(); ++i) {
            const PenAnchor& anchor = path.getAnchor(i);
            const gfx::Point& pos = anchor.position;

            bool isSelected = (i == m_state->m_tool.selectedAnchorIndex());
            // Anchor marker size: aim for ~6-8 screen pixels
            int screenSize = isSelected ? 8 : 6;
            int size = std::max(1, static_cast<int>(screenSize / zoomScale));
            int half = size / 2;
            if (half < 1) half = 1;

            gfx::Color fillColor = isSelected ? anchorColor : anchorFillColor;

            switch (anchor.type) {
                case AnchorType::Corner:
                    // Square for corner points
                    render->fillRect(fillColor,
                                   gfx::Rect(pos.x - half, pos.y - half, size, size));
                    render->drawRect(anchorColor,
                                   gfx::Rect(pos.x - half, pos.y - half, size, size));
                    break;

                case AnchorType::Smooth:
                    // Circle approximated with a filled rect and corner lines
                    render->fillRect(fillColor,
                                   gfx::Rect(pos.x - half + 1, pos.y - half, size - 2, size));
                    render->fillRect(fillColor,
                                   gfx::Rect(pos.x - half, pos.y - half + 1, size, size - 2));
                    render->drawRect(anchorColor,
                                   gfx::Rect(pos.x - half, pos.y - half, size, size));
                    break;

                case AnchorType::Symmetric:
                    // Diamond shape (rotated square)
                    render->drawLine(fillColor, pos.x, pos.y - half, pos.x + half, pos.y);
                    render->drawLine(fillColor, pos.x + half, pos.y, pos.x, pos.y + half);
                    render->drawLine(fillColor, pos.x, pos.y + half, pos.x - half, pos.y);
                    render->drawLine(fillColor, pos.x - half, pos.y, pos.x, pos.y - half);
                    // Fill center
                    render->fillRect(fillColor,
                                   gfx::Rect(pos.x - half/2, pos.y - half/2, half, half));
                    // Outline
                    render->drawLine(anchorColor, pos.x, pos.y - half, pos.x + half, pos.y);
                    render->drawLine(anchorColor, pos.x + half, pos.y, pos.x, pos.y + half);
                    render->drawLine(anchorColor, pos.x, pos.y + half, pos.x - half, pos.y);
                    render->drawLine(anchorColor, pos.x - half, pos.y, pos.x, pos.y - half);
                    break;
            }
        }

        // Draw pixel preview (in editor coordinates - will scale with zoom)
        std::vector<gfx::Point> previewPixels;
        if (m_state->m_mouseDown && !m_state->m_tool.isDraggingHandle()) {
            previewPixels = m_state->m_tool.generatePreviewWithTemp(
                editor->screenToEditor(m_state->m_lastMousePos));
        }
        else {
            previewPixels = m_state->m_tool.generatePreview();
        }

        for (const auto& pt : previewPixels) {
            // Pass editor coordinates directly - EditorPostRender converts to screen
            render->fillRect(previewColor, gfx::Rect(pt.x, pt.y, 1, 1));
        }
    }

    void getInvalidDecoratoredRegion(Editor* editor, gfx::Region& region) override {
        // Invalidate a large area around the path
        // This is a simplification - could be optimized
        const PenPath& path = m_state->m_tool.path();
        if (path.isEmpty()) {
            return;
        }

        gfx::Rect bounds = path.getBoundsWithHandles();
        bounds.enlarge(20);  // Add margin for handles and preview
        bounds = editor->editorToScreen(bounds);
        region |= gfx::Region(bounds);
    }

private:
    PixelPenState* m_state;
};

//------------------------------------------------------------------------------
// Constructor / Destructor
//------------------------------------------------------------------------------

PixelPenState::PixelPenState(Editor* editor)
    : m_lastMousePos(0, 0)
    , m_mouseDownPos(0, 0)
    , m_mouseDown(false)
    , m_creatingAnchor(false)
    , m_draggingAnchor(false)
    , m_clickedAnchorIdx(-1)
    , m_decorator(nullptr)
{
}

PixelPenState::~PixelPenState()
{
    delete m_decorator;
}

//------------------------------------------------------------------------------
// State lifecycle
//------------------------------------------------------------------------------

void PixelPenState::onEnterState(Editor* editor)
{
    StateWithWheelBehavior::onEnterState(editor);

    // Create and set decorator
    m_decorator = new Decorator(this);
    editor->setDecorator(m_decorator);
    editor->invalidate();
}

EditorState::LeaveAction PixelPenState::onLeaveState(Editor* editor, EditorState* newState)
{
    // Commit or cancel path before leaving
    if (!m_tool.path().isEmpty()) {
        // Ask user? For now, just cancel
        cancelPath(editor);
    }

    return DiscardState;
}

void PixelPenState::onBeforePopState(Editor* editor)
{
    editor->setDecorator(nullptr);
    editor->invalidate();
}

void PixelPenState::onActiveToolChange(Editor* editor, tools::Tool* tool)
{
    // If switching to a different tool, commit or cancel the path
    if (!m_tool.path().isEmpty()) {
        cancelPath(editor);
    }

    // Let the standby state handle the tool change
    StateWithWheelBehavior::onActiveToolChange(editor, tool);
}

//------------------------------------------------------------------------------
// Mouse events
//------------------------------------------------------------------------------

bool PixelPenState::onMouseDown(Editor* editor, MouseMessage* msg)
{
    if (msg->left()) {
        m_mouseDown = true;
        m_mouseDownPos = msg->position();
        m_lastMousePos = msg->position();

        gfx::Point spritePos = editor->screenToEditor(msg->position());

        // Edit Mode: Direct selection - move existing anchors/handles only
        // (activated via context bar toggle instead of Ctrl key)
        bool editMode = Preferences::instance().pixelPen.editMode();
        if (editMode) {
            int anchorIdx = hitTestAnchor(editor, msg->position());
            if (anchorIdx >= 0) {
                m_tool.selectAnchor(anchorIdx);
                m_draggingAnchor = true;
                m_clickedAnchorIdx = anchorIdx;  // Track for path closing
                invalidateEditor(editor);
                return true;
            }

            int handleType = 0;
            int handleAnchorIdx = hitTestHandle(editor, msg->position(), handleType);
            if (handleAnchorIdx >= 0) {
                m_tool.selectAnchor(handleAnchorIdx);
                m_tool.setDraggingHandle(true);
                m_tool.setDraggingHandleType(handleType);
                m_clickedAnchorIdx = -1;  // Not clicking an anchor
                invalidateEditor(editor);
                return true;
            }

            // In Edit mode, clicking empty space does nothing
            m_clickedAnchorIdx = -1;
            return true;
        }

        // Check if clicking on existing anchor - allow dragging to reposition
        int anchorIdx = hitTestAnchor(editor, msg->position());
        if (anchorIdx >= 0) {
            // Select the anchor and enable dragging
            m_tool.selectAnchor(anchorIdx);
            m_draggingAnchor = true;  // Allow repositioning by dragging

            // Store the clicked anchor index for potential path closing on mouse up
            // (we only close path on click, not drag)
            m_clickedAnchorIdx = anchorIdx;

            invalidateEditor(editor);
            return true;
        }

        // Check if clicking on a handle - allow dragging to adjust curves
        int handleType = 0;
        int handleAnchorIdx = hitTestHandle(editor, msg->position(), handleType);
        if (handleAnchorIdx >= 0) {
            m_tool.selectAnchor(handleAnchorIdx);
            m_tool.setDraggingHandle(true);
            m_tool.setDraggingHandleType(handleType);
            m_clickedAnchorIdx = -1;  // Not clicking an anchor point
            invalidateEditor(editor);
            return true;
        }

        // Add new anchor
        addAnchor(editor, msg->position());
        m_creatingAnchor = true;
        m_clickedAnchorIdx = -1;  // New anchor, not clicking existing
        return true;
    }
    else if (msg->right()) {
        // Right click: commit path or show context menu
        if (!m_tool.path().isEmpty()) {
            commitPath(editor);
        }
        return true;
    }

    return StateWithWheelBehavior::onMouseDown(editor, msg);
}

bool PixelPenState::onMouseMove(Editor* editor, MouseMessage* msg)
{
    m_lastMousePos = msg->position();

    if (m_mouseDown) {
        gfx::Point spritePos = editor->screenToEditor(msg->position());
        gfx::Point delta = msg->position() - m_mouseDownPos;

        // Edit mode drag anchor movement
        bool angleSnap = Preferences::instance().pixelPen.angleSnap();
        if (m_draggingAnchor) {
            int idx = m_tool.selectedAnchorIndex();
            if (idx >= 0) {
                gfx::Point newPos = spritePos;
                // Angle snap (45°) enabled via context bar toggle
                if (angleSnap && idx > 0) {
                    newPos = snapTo45Degrees(
                        m_tool.path().getAnchor(idx - 1).position, spritePos);
                }
                m_tool.path().moveAnchor(idx, newPos);
            }
            invalidateEditor(editor);
            return true;
        }

        // If dragging a handle
        if (m_tool.isDraggingHandle()) {
            int idx = m_tool.selectedAnchorIndex();
            if (idx >= 0) {
                PenAnchor& anchor = m_tool.path().getAnchor(idx);

                gfx::Point handlePos = spritePos;
                // Angle snap (45°) enabled via context bar toggle
                if (angleSnap) {
                    handlePos = snapTo45Degrees(anchor.position, spritePos);
                }

                // Apply anchor type from preference (Corner makes handles independent)
                int prefAnchorType = Preferences::instance().pixelPen.anchorType();
                anchor.type = static_cast<AnchorType>(prefAnchorType);

                if (m_tool.draggingHandleType() == 1) {
                    anchor.setHandleInAbsolute(handlePos);
                }
                else if (m_tool.draggingHandleType() == 2) {
                    anchor.setHandleOutAbsolute(handlePos);
                }
            }
            invalidateEditor(editor);
            return true;
        }

        // If creating a new anchor, drag creates the handle
        if (m_creatingAnchor && m_tool.path().anchorCount() > 0) {
            // Check if dragged far enough to create handles
            int distance = std::abs(delta.x) + std::abs(delta.y);
            if (distance > 3) {
                int lastIdx = m_tool.path().anchorCount() - 1;
                PenAnchor& anchor = m_tool.path().getAnchor(lastIdx);

                gfx::Point handleTarget = spritePos;
                // Angle snap (45°) enabled via context bar toggle
                if (angleSnap) {
                    handleTarget = snapTo45Degrees(anchor.position, spritePos);
                }

                // Set the outgoing handle (drag direction)
                gfx::Point handleOffset = handleTarget - anchor.position;
                anchor.handleOut = handleOffset;
                // Mirror for incoming handle
                anchor.handleIn = gfx::Point(-handleOffset.x, -handleOffset.y);
                // Use anchor type from preference (default to Smooth when dragging)
                int prefAnchorType = Preferences::instance().pixelPen.anchorType();
                anchor.type = static_cast<AnchorType>(prefAnchorType);
                // If not Corner, default to Smooth when creating handles
                if (anchor.type == AnchorType::Corner && (anchor.hasHandleIn() || anchor.hasHandleOut())) {
                    anchor.type = AnchorType::Smooth;
                }
            }
            invalidateEditor(editor);
            return true;
        }
    }

    // Update preview even when not dragging
    invalidateEditor(editor);

    return StateWithWheelBehavior::onMouseMove(editor, msg);
}

bool PixelPenState::onMouseUp(Editor* editor, MouseMessage* msg)
{
    // Check if this was a click on the first anchor (to close path)
    // Only close if mouse didn't move much (it was a click, not a drag)
    if (m_clickedAnchorIdx == 0 && m_tool.path().canClose()) {
        gfx::Point delta = msg->position() - m_mouseDownPos;
        int moveDistance = std::abs(delta.x) + std::abs(delta.y);
        if (moveDistance < 5) {  // Threshold for "click" vs "drag"
            closePath(editor);
        }
    }

    m_mouseDown = false;
    m_creatingAnchor = false;
    m_draggingAnchor = false;
    m_clickedAnchorIdx = -1;
    m_tool.setDraggingHandle(false);
    m_tool.setDraggingHandleType(0);

    invalidateEditor(editor);
    return true;
}

bool PixelPenState::onDoubleClick(Editor* editor, MouseMessage* msg)
{
    // Double-click commits the path
    if (!m_tool.path().isEmpty()) {
        commitPath(editor);
    }
    return true;
}

//------------------------------------------------------------------------------
// Keyboard events
//------------------------------------------------------------------------------

bool PixelPenState::onKeyDown(Editor* editor, KeyMessage* msg)
{
    switch (msg->scancode()) {
        case kKeyEnter:
        case kKeyEnterPad:
            // Commit path
            if (!m_tool.path().isEmpty()) {
                commitPath(editor);
            }
            return true;

        case kKeyEsc:
            // Cancel path
            cancelPath(editor);
            return true;

        case kKeyBackspace:
            // Delete last anchor
            deleteLastAnchor(editor);
            return true;

        case kKeyDel:
            // Delete selected anchor
            deleteSelectedAnchor(editor);
            return true;

        case kKeyC:
            // Toggle closed path
            if (m_tool.path().canClose()) {
                m_tool.path().toggleClosed();
                invalidateEditor(editor);
            }
            return true;

        case kKeyS:
            // Toggle smooth/corner for selected anchor
            if (m_tool.hasSelection()) {
                m_tool.path().getAnchor(m_tool.selectedAnchorIndex()).toggleSmooth();
                invalidateEditor(editor);
            }
            return true;

        default:
            break;
    }

    return StateWithWheelBehavior::onKeyDown(editor, msg);
}

bool PixelPenState::onKeyUp(Editor* editor, KeyMessage* msg)
{
    return StateWithWheelBehavior::onKeyUp(editor, msg);
}

//------------------------------------------------------------------------------
// Cursor
//------------------------------------------------------------------------------

bool PixelPenState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
    // Check for anchor hit
    if (hitTestAnchor(editor, mouseScreenPos) >= 0) {
        editor->showMouseCursor(kMoveCursor);
        return true;
    }

    // Check for handle hit
    int handleType = 0;
    if (hitTestHandle(editor, mouseScreenPos, handleType) >= 0) {
        editor->showMouseCursor(kMoveCursor);
        return true;
    }

    // Check for close path hit
    if (hitTestClosePath(editor, mouseScreenPos)) {
        editor->showMouseCursor(kHandCursor);
        return true;
    }

    // Default pen cursor
    editor->showMouseCursor(kArrowPlusCursor);
    return true;
}

//------------------------------------------------------------------------------
// Status bar
//------------------------------------------------------------------------------

bool PixelPenState::onUpdateStatusBar(Editor* editor)
{
    gfx::Point spritePos = editor->screenToEditor(m_lastMousePos);

    bool editMode = Preferences::instance().pixelPen.editMode();
    bool angleSnap = Preferences::instance().pixelPen.angleSnap();
    const char* editModeStr = editMode ? "ON" : "OFF";
    const char* angleSnapStr = angleSnap ? "ON" : "OFF";

    if (m_tool.hasSelection()) {
        int idx = m_tool.selectedAnchorIndex();
        const PenAnchor& anchor = m_tool.path().getAnchor(idx);
        StatusBar::instance()->setStatusText(
            0,
            fmt::format("Pixel Pen: Anchor {}/{} [{}] | Edit:{} 45deg:{} | "
            "Enter=commit, Esc=cancel, Del=delete",
            idx + 1, m_tool.path().anchorCount(), anchor.getTypeName(),
            editModeStr, angleSnapStr));
    }
    else {
        StatusBar::instance()->setStatusText(
            0,
            fmt::format("Pixel Pen: {} anchors | Click=add, Drag=curves | "
            "Edit:{} 45deg:{} | Enter=commit",
            m_tool.path().anchorCount(), editModeStr, angleSnapStr));
    }

    return true;
}

//------------------------------------------------------------------------------
// Drawing
//------------------------------------------------------------------------------

void PixelPenState::onExposeSpritePixels(const gfx::Region& rgn)
{
    // The decorator handles the rendering
}

//------------------------------------------------------------------------------
// Hit testing
//------------------------------------------------------------------------------

int PixelPenState::hitTestAnchor(Editor* editor, const gfx::Point& screenPos, int threshold)
{
    const PenPath& path = m_tool.path();

    for (int i = 0; i < path.anchorCount(); ++i) {
        gfx::Point anchorScreen = editor->editorToScreen(path.getAnchor(i).position);
        int dist = std::abs(screenPos.x - anchorScreen.x) +
                   std::abs(screenPos.y - anchorScreen.y);
        if (dist <= threshold) {
            return i;
        }
    }

    return -1;
}

int PixelPenState::hitTestHandle(Editor* editor, const gfx::Point& screenPos,
                                  int& handleType, int threshold)
{
    const PenPath& path = m_tool.path();

    for (int i = 0; i < path.anchorCount(); ++i) {
        const PenAnchor& anchor = path.getAnchor(i);

        if (anchor.hasHandleIn()) {
            gfx::Point handleScreen = editor->editorToScreen(anchor.getHandleInAbsolute());
            int dist = std::abs(screenPos.x - handleScreen.x) +
                       std::abs(screenPos.y - handleScreen.y);
            if (dist <= threshold) {
                handleType = 1;
                return i;
            }
        }

        if (anchor.hasHandleOut()) {
            gfx::Point handleScreen = editor->editorToScreen(anchor.getHandleOutAbsolute());
            int dist = std::abs(screenPos.x - handleScreen.x) +
                       std::abs(screenPos.y - handleScreen.y);
            if (dist <= threshold) {
                handleType = 2;
                return i;
            }
        }
    }

    handleType = 0;
    return -1;
}

bool PixelPenState::hitTestClosePath(Editor* editor, const gfx::Point& screenPos, int threshold)
{
    const PenPath& path = m_tool.path();

    if (!path.canClose() || path.isClosed()) {
        return false;
    }

    gfx::Point firstAnchorScreen = editor->editorToScreen(path.firstAnchor().position);
    int dist = std::abs(screenPos.x - firstAnchorScreen.x) +
               std::abs(screenPos.y - firstAnchorScreen.y);

    return dist <= threshold;
}

//------------------------------------------------------------------------------
// Actions
//------------------------------------------------------------------------------

void PixelPenState::addAnchor(Editor* editor, const gfx::Point& screenPos)
{
    gfx::Point spritePos = editor->screenToEditor(screenPos);
    // Get anchor type from context bar preference
    int prefAnchorType = Preferences::instance().pixelPen.anchorType();
    AnchorType anchorType = static_cast<AnchorType>(prefAnchorType);
    m_tool.path().addAnchor(PenAnchor(spritePos, anchorType));
    m_tool.selectAnchor(m_tool.path().anchorCount() - 1);
    m_tool.setDrawing(true);
    invalidateEditor(editor);
}

void PixelPenState::closePath(Editor* editor)
{
    m_tool.path().setClosed(true);
    invalidateEditor(editor);
}

void PixelPenState::commitPath(Editor* editor)
{
    if (m_tool.path().isEmpty()) {
        return;
    }

    // Get the current sprite and layer
    doc::Sprite* sprite = editor->sprite();
    doc::Layer* layer = editor->layer();

    if (!sprite || !layer || !layer->isImage()) {
        cancelPath(editor);
        return;
    }

    // Get the active cel's image
    doc::Cel* cel = layer->cel(editor->frame());
    if (!cel) {
        // Could create a new cel here, but for simplicity just cancel
        cancelPath(editor);
        return;
    }

    doc::Image* image = cel->image();
    if (!image) {
        cancelPath(editor);
        return;
    }

    // Get the foreground color
    app::Color appColor = Preferences::instance().colorBar.fgColor();
    doc::color_t color = color_utils::color_for_layer(appColor, layer);

    // Generate pixels to draw
    std::vector<gfx::Point> pixels = m_tool.generatePreview();
    if (pixels.empty()) {
        cancelPath(editor);
        return;
    }

    // Save the path before committing (for restore feature)
    m_tool.saveLastPath();

    // Create an undoable transaction
    Site site = editor->getSite();
    Doc* doc = site.document();

    {
        Tx tx(doc, "Pixel Pen Path");
        ExpandCelCanvas expand(site, layer, TiledMode::NONE, tx, ExpandCelCanvas::None);

        // Calculate the region we need to modify
        gfx::Rect bounds;
        for (const auto& pt : pixels) {
            if (bounds.isEmpty()) {
                bounds = gfx::Rect(pt.x, pt.y, 1, 1);
            } else {
                bounds |= gfx::Rect(pt.x, pt.y, 1, 1);
            }
        }

        // Get cel origin offset
        gfx::Point celOrigin = expand.getCelOrigin();

        // Validate the destination region
        gfx::Region rgn(gfx::Rect(bounds.x - celOrigin.x,
                                   bounds.y - celOrigin.y,
                                   bounds.w, bounds.h));
        expand.validateDestCanvas(rgn);

        // Get the destination canvas and draw pixels
        doc::Image* dstImage = expand.getDestCanvas();
        if (dstImage) {
            for (const auto& pt : pixels) {
                int x = pt.x - celOrigin.x;
                int y = pt.y - celOrigin.y;
                if (x >= 0 && x < dstImage->width() &&
                    y >= 0 && y < dstImage->height()) {
                    doc::put_pixel(dstImage, x, y, color);
                }
            }
        }

        expand.commit();
        tx.commit();
    }

    // Reset tool (but keep last path for restore)
    m_tool.path().clear();
    m_tool.clearSelection();
    m_tool.setDrawing(false);

    // Notify document change
    doc->notifyGeneralUpdate();

    // Invalidate the editor to show the changes
    editor->invalidate();
}

void PixelPenState::cancelPath(Editor* editor)
{
    m_tool.reset();
    invalidateEditor(editor);
}

void PixelPenState::restoreLastPath(Editor* editor)
{
    if (!m_tool.hasLastPath()) {
        return;
    }

    // First, undo the last committed pixels
    Command* undoCmd = Commands::instance()->byId(CommandId::Undo());
    if (undoCmd) {
        UIContext::instance()->executeCommand(undoCmd);
    }

    // Then restore the path for editing
    m_tool.restoreLastPath();
    invalidateEditor(editor);
}

void PixelPenState::addHandlesToSelectedAnchor(Editor* editor)
{
    if (!canAddHandles()) {
        return;
    }

    // Create default handles on the selected anchor
    if (m_tool.createDefaultHandles()) {
        invalidateEditor(editor);
    }
}

void PixelPenState::deleteSelectedAnchor(Editor* editor)
{
    if (m_tool.hasSelection()) {
        int idx = m_tool.selectedAnchorIndex();
        m_tool.path().removeAnchor(idx);
        m_tool.clearSelection();
        invalidateEditor(editor);
    }
}

void PixelPenState::deleteLastAnchor(Editor* editor)
{
    if (!m_tool.path().isEmpty()) {
        m_tool.path().removeLastAnchor();
        // Select the new last anchor if any
        if (!m_tool.path().isEmpty()) {
            m_tool.selectAnchor(m_tool.path().anchorCount() - 1);
        }
        else {
            m_tool.clearSelection();
        }
        invalidateEditor(editor);
    }
}

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

void PixelPenState::invalidateEditor(Editor* editor)
{
    editor->invalidate();
}

gfx::Point PixelPenState::snapTo45Degrees(const gfx::Point& origin, const gfx::Point& target)
{
    double dx = target.x - origin.x;
    double dy = target.y - origin.y;
    double angle = std::atan2(dy, dx);
    double distance = std::sqrt(dx * dx + dy * dy);

    // Snap to nearest 45° (π/4 radians)
    double snapped = std::round(angle / (M_PI / 4)) * (M_PI / 4);

    return gfx::Point(
        origin.x + static_cast<int>(std::cos(snapped) * distance),
        origin.y + static_cast<int>(std::sin(snapped) * distance)
    );
}

} // namespace app
