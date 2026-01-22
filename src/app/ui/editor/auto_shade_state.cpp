// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade State - Implementation

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "auto_shade_state.h"

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/i18n/strings.h"
#include "app/pref/preferences.h"
#include "app/site.h"
#include "app/tools/tool_box.h"
#include "app/tx.h"
#include "app/ui/color_bar.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/editor_decorator.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "app/util/expand_cel_canvas.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "gfx/region.h"
#include "ui/keys.h"
#include "ui/message.h"
#include "ui/system.h"

#include "fmt/format.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

// Debug logging
#define SHADE_LOG(fmt, ...) printf("[AutoShade] " fmt "\n", ##__VA_ARGS__)

namespace app {

using namespace ui;
using namespace tools;

//------------------------------------------------------------------------------
// Decorator - Renders the shading preview overlay
//------------------------------------------------------------------------------

class AutoShadeState::Decorator : public EditorDecorator {
public:
    Decorator(AutoShadeState* state) : m_state(state) {}

    void postRenderDecorator(EditorPostRender* render) override {
        if (!m_state->m_hasPreview || m_state->m_previewColors.empty()) {
            return;
        }

        // Draw each preview pixel
        for (const auto& pair : m_state->m_previewColors) {
            const gfx::Point& pos = pair.first;
            doc::color_t color = pair.second;

            // Convert doc::color_t to gfx::Color for rendering
            gfx::Color gfxColor = gfx::rgba(
                doc::rgba_getr(color),
                doc::rgba_getg(color),
                doc::rgba_getb(color),
                doc::rgba_geta(color)
            );

            // Draw a single pixel rectangle (EditorPostRender handles coord conversion)
            render->fillRect(gfxColor, gfx::Rect(pos.x, pos.y, 1, 1));
        }

        // Draw region outline (if enabled)
        if (m_state->m_tool.config().showOutline && !m_state->m_tool.lastShape().isEmpty()) {
            const ShapeData& shape = m_state->m_tool.lastShape();
            const ShadeConfig& config = m_state->m_tool.config();

            // Calculate light direction for selective outlining
            double angleRad = config.lightAngle * (3.14159265358979323846 / 180.0);
            double lightX = std::cos(angleRad);
            double lightY = -std::sin(angleRad);

            // Draw edge pixels outline with selective coloring
            for (const auto& edge : shape.edgePixels) {
                gfx::Color outlineColor;

                if (config.enableSelectiveOutline) {
                    // Calculate direction from center to this edge pixel
                    double dx = edge.x - shape.centerX;
                    double dy = edge.y - shape.centerY;
                    double len = std::sqrt(dx * dx + dy * dy);
                    if (len > 0.001) {
                        dx /= len;
                        dy /= len;
                    }

                    // Dot product with light direction
                    // Positive = facing light, Negative = facing away
                    double dotLight = dx * lightX + dy * lightY;

                    if (dotLight > 0.1) {
                        // Light side - use light outline color
                        outlineColor = gfx::rgba(
                            doc::rgba_getr(config.lightOutlineColor),
                            doc::rgba_getg(config.lightOutlineColor),
                            doc::rgba_getb(config.lightOutlineColor),
                            180);
                    }
                    else if (dotLight < -0.1) {
                        // Shadow side - use shadow outline color
                        outlineColor = gfx::rgba(
                            doc::rgba_getr(config.shadowOutlineColor),
                            doc::rgba_getg(config.shadowOutlineColor),
                            doc::rgba_getb(config.shadowOutlineColor),
                            180);
                    }
                    else {
                        // Transition zone - blend between colors
                        double t = (dotLight + 0.1) / 0.2;  // 0 to 1
                        int r = static_cast<int>(
                            doc::rgba_getr(config.shadowOutlineColor) * (1.0 - t) +
                            doc::rgba_getr(config.lightOutlineColor) * t);
                        int g = static_cast<int>(
                            doc::rgba_getg(config.shadowOutlineColor) * (1.0 - t) +
                            doc::rgba_getg(config.lightOutlineColor) * t);
                        int b = static_cast<int>(
                            doc::rgba_getb(config.shadowOutlineColor) * (1.0 - t) +
                            doc::rgba_getb(config.lightOutlineColor) * t);
                        outlineColor = gfx::rgba(r, g, b, 180);
                    }
                }
                else {
                    // Default yellow outline
                    outlineColor = gfx::rgba(255, 255, 0, 180);
                }

                render->drawRect(outlineColor, gfx::Rect(edge.x, edge.y, 1, 1));
            }
        }

        // Draw light direction handle and highlight point
        if (!m_state->m_tool.lastShape().isEmpty()) {
            const ShapeData& shape = m_state->m_tool.lastShape();
            const ShadeConfig& config = m_state->m_tool.config();

            const double PI = 3.14159265358979323846;
            double angleRad = config.lightAngle * (PI / 180.0);

            // Calculate sun handle distance based on elevation
            // elevation 0° = at edge (minDist), elevation 90° = far away (maxDist)
            double minDist = shape.maxDistance;
            double maxDist = shape.maxDistance * 3.0;
            double elevationFactor = config.lightElevation / 90.0;
            if (elevationFactor > 1.0) elevationFactor = 1.0;
            double handleDist = minDist + (maxDist - minDist) * elevationFactor;

            // Light source position (where light comes FROM)
            int handleX = static_cast<int>(shape.centerX + std::cos(angleRad) * handleDist);
            int handleY = static_cast<int>(shape.centerY - std::sin(angleRad) * handleDist);

            int centerX = static_cast<int>(shape.centerX);
            int centerY = static_cast<int>(shape.centerY);

            // --- Draw highlight point (where light hits the shape) ---
            // This is freely movable on the shape surface (independent of light direction)
            // It's draggable to control where the brightest area appears
            double highlightAngleRad = m_state->m_highlightAngle * (PI / 180.0);
            double highlightDist = shape.maxDistance * m_state->m_highlightDistanceRatio;
            int highlightX = static_cast<int>(shape.centerX + std::cos(highlightAngleRad) * highlightDist);
            int highlightY = static_cast<int>(shape.centerY - std::sin(highlightAngleRad) * highlightDist);

            // Draw highlight point as a small bright circle with outline
            // Changes color when dragging to show it's interactive
            gfx::Color highlightColor = m_state->m_draggingHighlightPoint ?
                gfx::rgba(255, 255, 150, 255) :  // Bright yellow when dragging
                gfx::rgba(255, 255, 255, 240);   // White normally
            gfx::Color highlightOutline = gfx::rgba(0, 0, 0, 180);  // Dark outline

            // Draw outer ring (outline) - radius 2
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    int distSq = dx * dx + dy * dy;
                    if (distSq > 1 && distSq <= 4) {
                        render->fillRect(highlightOutline,
                            gfx::Rect(highlightX + dx, highlightY + dy, 1, 1));
                    }
                }
            }
            // Draw inner circle - radius 1
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx * dx + dy * dy <= 1) {
                        render->fillRect(highlightColor,
                            gfx::Rect(highlightX + dx, highlightY + dy, 1, 1));
                    }
                }
            }

            // --- Draw line from highlight to sun handle ---
            gfx::Color lineColor = gfx::rgba(255, 200, 0, 180);  // Golden yellow

            // Draw dashed line from highlight point to sun
            int segments = 12;
            for (int i = 0; i < segments; i += 2) {
                double t1 = static_cast<double>(i) / segments;
                double t2 = static_cast<double>(i + 1) / segments;
                int x1 = highlightX + static_cast<int>((handleX - highlightX) * t1);
                int y1 = highlightY + static_cast<int>((handleY - highlightY) * t1);
                int x2 = highlightX + static_cast<int>((handleX - highlightX) * t2);
                int y2 = highlightY + static_cast<int>((handleY - highlightY) * t2);

                render->fillRect(lineColor, gfx::Rect(x1, y1, 1, 1));
                render->fillRect(lineColor, gfx::Rect(x2, y2, 1, 1));
            }

            // --- Draw the sun handle ---
            gfx::Color handleColor = m_state->m_draggingLightHandle ?
                gfx::rgba(255, 255, 100, 255) :  // Bright when dragging
                gfx::rgba(255, 200, 0, 255);     // Golden yellow normally

            // Draw a small sun/circle at handle position
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    if (dx * dx + dy * dy <= 4) {
                        render->fillRect(handleColor, gfx::Rect(handleX + dx, handleY + dy, 1, 1));
                    }
                }
            }

            // Draw rays extending from the sun
            gfx::Color rayColor = gfx::rgba(255, 220, 50, 180);
            for (int i = 0; i < 8; ++i) {
                double rayAngle = i * (PI / 4);
                int rayX = handleX + static_cast<int>(std::cos(rayAngle) * 4);
                int rayY = handleY + static_cast<int>(std::sin(rayAngle) * 4);
                render->fillRect(rayColor, gfx::Rect(rayX, rayY, 1, 1));
            }

            // Draw elevation indicator text near sun (show angle)
            // (Visual feedback - the distance represents elevation)
        }
    }

    void getInvalidDecoratoredRegion(Editor* editor, gfx::Region& region) override {
        if (!m_state->m_hasPreview || m_state->m_tool.lastShape().isEmpty()) {
            return;
        }

        // Add the shape bounds to the invalidated region
        const ShapeData& shape = m_state->m_tool.lastShape();
        gfx::Rect bounds = shape.bounds;

        // Expand to include light handle (1.5x max radius + handle size)
        int handleMargin = static_cast<int>(shape.maxDistance * 0.6) + 10;
        bounds.enlarge(handleMargin);

        // Convert to screen coordinates
        gfx::Rect screenBounds = editor->editorToScreen(bounds);
        region |= gfx::Region(screenBounds);
    }

private:
    AutoShadeState* m_state;
};

//------------------------------------------------------------------------------
// AutoShadeState - Constructor/Destructor
//------------------------------------------------------------------------------

AutoShadeState::AutoShadeState(Editor* editor)
    : m_hasPreview(false)
    , m_mouseDown(false)
    , m_draggingLightHandle(false)
    , m_draggingHighlightPoint(false)
    , m_highlightDistanceRatio(0.3)  // Default: 30% from center toward edge
    , m_highlightAngle(135.0)        // Default: match default light angle
    , m_decorator(nullptr)
{
    // Initialize with default config
    // Could load from preferences here
}

AutoShadeState::~AutoShadeState()
{
    delete m_decorator;
}

//------------------------------------------------------------------------------
// State interface
//------------------------------------------------------------------------------

void AutoShadeState::onEnterState(Editor* editor)
{
    StateWithWheelBehavior::onEnterState(editor);

    // Create and set decorator
    m_decorator = new Decorator(this);
    editor->setDecorator(m_decorator);

    // Load colors from color bar based on color source setting
    ColorBar* colorBar = ColorBar::instance();
    if (colorBar) {
        app::Color sourceColor;
        if (m_tool.config().colorSource == tools::ColorSource::Background) {
            sourceColor = colorBar->getBgColor();
        } else {
            sourceColor = colorBar->getFgColor();
        }

        // Use selected color as base color (always opaque for shading)
        int r = sourceColor.getRed();
        int g = sourceColor.getGreen();
        int b = sourceColor.getBlue();

        m_tool.config().baseColor = doc::rgba(r, g, b, 255);

        // Shadow: darker, shift toward blue
        m_tool.config().shadowColor = doc::rgba(
            std::max(0, r - 60),
            std::max(0, g - 60),
            std::max(0, b - 40),
            255);

        // Highlight: brighter, shift toward yellow
        m_tool.config().highlightColor = doc::rgba(
            std::min(255, r + 60),
            std::min(255, g + 50),
            std::min(255, b + 30),
            255);
    }

    // Notify context bar to update colors from this state
    if (auto* mainWindow = App::instance()->mainWindow()) {
        if (auto* contextBar = mainWindow->getContextBar()) {
            contextBar->updateForActiveTool();
        }
    }

    editor->invalidate();
}

EditorState::LeaveAction AutoShadeState::onLeaveState(Editor* editor, EditorState* newState)
{
    // Cancel any pending preview
    cancelPreview(editor);

    // Clear decorator (will be deleted in destructor)
    if (m_decorator) {
        editor->setDecorator(nullptr);
    }

    return LeaveAction::DiscardState;
}

void AutoShadeState::onBeforePopState(Editor* editor)
{
    // Clean up
    if (m_decorator) {
        editor->setDecorator(nullptr);
        delete m_decorator;
        m_decorator = nullptr;
    }

    StateWithWheelBehavior::onBeforePopState(editor);
}

void AutoShadeState::onActiveToolChange(Editor* editor, tools::Tool* tool)
{
    // If tool changes away from auto_shade, pop this state
    if (tool && tool->getId() != "auto_shade") {
        cancelPreview(editor);
        editor->backToPreviousState();
    }
}

//------------------------------------------------------------------------------
// Mouse events
//------------------------------------------------------------------------------

bool AutoShadeState::onMouseDown(Editor* editor, ui::MouseMessage* msg)
{
    if (msg->left()) {
        m_mouseDown = true;
        m_lastMousePos = msg->position();

        // Check if clicking on the highlight point (incidence point)
        if (m_hasPreview && hitTestHighlightPoint(editor, msg->position())) {
            m_draggingHighlightPoint = true;
            return true;
        }

        // Check if clicking on the light handle (sun)
        if (m_hasPreview && hitTestLightHandle(editor, msg->position())) {
            m_draggingLightHandle = true;
            return true;
        }

        // Convert screen position to sprite coordinates
        gfx::Point spritePos = editor->screenToEditor(msg->position());
        m_clickPos = spritePos;

        // Analyze the region at this position
        analyzeRegion(editor, spritePos);

        return true;
    }

    return StateWithWheelBehavior::onMouseDown(editor, msg);
}

bool AutoShadeState::onMouseMove(Editor* editor, ui::MouseMessage* msg)
{
    m_lastMousePos = msg->position();

    // Handle dragging the highlight/incidence point
    // This allows free 2D movement - both angle and distance can change
    if (m_draggingHighlightPoint && m_hasPreview && !m_tool.lastShape().isEmpty()) {
        const ShapeData& shape = m_tool.lastShape();

        // Convert mouse position to sprite coordinates
        gfx::Point spritePos = editor->screenToEditor(msg->position());

        // Calculate vector from center to mouse
        double dx = spritePos.x - shape.centerX;
        double dy = spritePos.y - shape.centerY;
        double distance = std::sqrt(dx * dx + dy * dy);

        // Calculate angle (atan2 gives -PI to PI, we want 0 to 360)
        // Note: Y is inverted in screen coords, so negate dy
        const double PI = 3.14159265358979323846;
        double angle = std::atan2(-dy, dx) * (180.0 / PI);
        if (angle < 0) angle += 360.0;

        // Update the highlight angle (independent of light angle)
        m_highlightAngle = angle;

        // Update the distance ratio (0.0 = center, 1.0 = edge)
        // Clamp to 0.05-0.95 to keep highlight visible and within shape
        double maxDist = shape.maxDistance;
        if (maxDist > 0.001) {
            m_highlightDistanceRatio = distance / maxDist;
            m_highlightDistanceRatio = std::max(0.05, std::min(0.95, m_highlightDistanceRatio));
        }

        // Regenerate the preview
        updatePreview(editor);

        return true;
    }

    // Handle dragging the light handle (sun)
    if (m_draggingLightHandle && m_hasPreview && !m_tool.lastShape().isEmpty()) {
        const ShapeData& shape = m_tool.lastShape();

        // Convert mouse position to sprite coordinates
        gfx::Point spritePos = editor->screenToEditor(msg->position());

        // Calculate angle and distance from shape center to mouse position
        double dx = spritePos.x - shape.centerX;
        double dy = spritePos.y - shape.centerY;
        double distance = std::sqrt(dx * dx + dy * dy);

        // Calculate angle (atan2 gives -PI to PI, we want 0 to 360)
        // Note: Y is inverted in screen coords, so negate dy
        double angle = std::atan2(-dy, dx) * (180.0 / 3.14159265358979323846);
        if (angle < 0) angle += 360.0;

        // Update the light angle
        m_tool.config().lightAngle = angle;

        // Calculate elevation from distance:
        // - At shape edge (distance = maxRadius): elevation = 0° (frontal)
        // - At 3x maxRadius: elevation = 90° (from above)
        // This gives intuitive control: pull sun away = light from above
        double minDist = shape.maxDistance;  // Minimum: at edge
        double maxDist = shape.maxDistance * 3.0;  // Maximum: 3x the radius
        double normalizedDist = (distance - minDist) / (maxDist - minDist);
        normalizedDist = std::max(0.0, std::min(1.0, normalizedDist));

        // Map to elevation angle (0-90 degrees, clamped)
        double elevation = normalizedDist * 90.0;
        m_tool.config().lightElevation = elevation;

        // Regenerate the preview
        updatePreview(editor);

        return true;
    }

    // Update status bar with position
    editor->updateStatusBar();

    return true;
}

bool AutoShadeState::onMouseUp(Editor* editor, ui::MouseMessage* msg)
{
    if (msg->left() && m_mouseDown) {
        m_mouseDown = false;
        m_draggingLightHandle = false;
        m_draggingHighlightPoint = false;
        return true;
    }

    return StateWithWheelBehavior::onMouseUp(editor, msg);
}

bool AutoShadeState::onDoubleClick(Editor* editor, ui::MouseMessage* msg)
{
    // Double-click applies the shading
    if (msg->left() && m_hasPreview) {
        applyShading(editor);
        return true;
    }

    return StateWithWheelBehavior::onDoubleClick(editor, msg);
}

//------------------------------------------------------------------------------
// Keyboard events
//------------------------------------------------------------------------------

bool AutoShadeState::onKeyDown(Editor* editor, ui::KeyMessage* msg)
{
    switch (msg->scancode()) {
        case kKeyEnter:
        case kKeyEnterPad:
            // Apply shading
            if (m_hasPreview) {
                applyShading(editor);
                return true;
            }
            break;

        case kKeyEsc:
            // Cancel preview and return to standby
            cancelPreview(editor);
            editor->backToPreviousState();
            return true;

        case kKeyLeft:
            // Rotate light angle counter-clockwise
            m_tool.config().lightAngle -= 15.0;
            if (m_tool.config().lightAngle < 0)
                m_tool.config().lightAngle += 360.0;
            updatePreview(editor);
            return true;

        case kKeyRight:
            // Rotate light angle clockwise
            m_tool.config().lightAngle += 15.0;
            if (m_tool.config().lightAngle >= 360.0)
                m_tool.config().lightAngle -= 360.0;
            updatePreview(editor);
            return true;

        case kKeyUp:
            // Increase ambient light
            m_tool.config().ambientLevel = std::min(1.0, m_tool.config().ambientLevel + 0.05);
            updatePreview(editor);
            return true;

        case kKeyDown:
            // Decrease ambient light
            m_tool.config().ambientLevel = std::max(0.0, m_tool.config().ambientLevel - 0.05);
            updatePreview(editor);
            return true;
    }

    return StateWithWheelBehavior::onKeyDown(editor, msg);
}

bool AutoShadeState::onKeyUp(Editor* editor, ui::KeyMessage* msg)
{
    return StateWithWheelBehavior::onKeyUp(editor, msg);
}

//------------------------------------------------------------------------------
// Cursor
//------------------------------------------------------------------------------

bool AutoShadeState::onSetCursor(Editor* editor, const gfx::Point& mouseScreenPos)
{
    // Show move cursor when hovering over the highlight/incidence point
    if (m_hasPreview && hitTestHighlightPoint(editor, mouseScreenPos)) {
        editor->showMouseCursor(kMoveCursor);
        return true;
    }

    // Show move cursor when hovering over the light handle (sun)
    if (m_hasPreview && hitTestLightHandle(editor, mouseScreenPos)) {
        editor->showMouseCursor(kMoveCursor);
        return true;
    }

    // Use crosshair cursor for region selection
    editor->showMouseCursor(kCrosshairCursor);
    return true;
}

//------------------------------------------------------------------------------
// Status bar
//------------------------------------------------------------------------------

bool AutoShadeState::onUpdateStatusBar(Editor* editor)
{
    gfx::Point spritePos = editor->screenToEditor(m_lastMousePos);

    std::string statusText;
    if (m_hasPreview) {
        statusText = fmt::format(
            "Auto-Shade: Light: {:.0f}° Elev: {:.0f}° | Drag highlight or sun | Enter=apply, Esc=cancel",
            m_tool.config().lightAngle,
            m_tool.config().lightElevation);
    }
    else {
        statusText = fmt::format(
            "Auto-Shade: Click on a region to shade | Pos: ({}, {})",
            spritePos.x, spritePos.y);
    }

    StatusBar::instance()->setStatusText(0, statusText);
    return true;
}

//------------------------------------------------------------------------------
// Drawing
//------------------------------------------------------------------------------

void AutoShadeState::onExposeSpritePixels(const gfx::Region& rgn)
{
    // Nothing special needed here
}

//------------------------------------------------------------------------------
// Configuration and preview
//------------------------------------------------------------------------------

void AutoShadeState::updatePreview(Editor* editor)
{
    if (!m_hasPreview) {
        return;
    }

    // Regenerate preview with new config using original colors
    generateShadedPreview();

    invalidateEditor(editor);
}

void AutoShadeState::generateShadedPreview()
{
    m_previewColors.clear();

    if (m_tool.lastShape().isEmpty() || m_originalColors.empty()) {
        SHADE_LOG("generateShadedPreview: ERROR - empty shape or no original colors");
        return;
    }

    const ShapeData& shape = m_tool.lastShape();
    const ShadeConfig& config = m_tool.config();

    // =========================================================================
    // STEP 1: Calculate light direction from angle (3D light vector)
    // Angle convention: 0° = right, 90° = up, 180° = left, 270° = down
    // Elevation: 0° = front (horizontal), 90° = top, 180° = back
    // Light points FROM the light source TOWARD the shape
    // =========================================================================
    const double PI = 3.14159265358979323846;
    double angleRad = config.lightAngle * (PI / 180.0);
    double elevationRad = config.lightElevation * (PI / 180.0);

    // Calculate 3D light direction using spherical coordinates
    // Z component: how much light comes from "above" (peaks at 90°)
    double lightZ = std::sin(elevationRad);
    // Horizontal scale: 1 at 0°, 0 at 90°, -1 at 180° (behind)
    double horizontalScale = std::cos(elevationRad);

    // 2D light direction (XY plane), scaled by horizontal component
    double lightX = std::cos(angleRad) * horizontalScale;
    double lightY = -std::sin(angleRad) * horizontalScale;  // Negate for screen coords (Y down)

    // Calculate max radius for normalizing distances
    double maxRadius = shape.maxDistance;
    if (maxRadius < 1.0) maxRadius = 1.0;  // Prevent division by zero

    SHADE_LOG("generateShadedPreview: lightAngle=%.0f lightDir=(%.2f, %.2f, %.2f) roundness=%.2f maxRadius=%.1f",
              config.lightAngle, lightX, lightY, lightZ, config.roundness, maxRadius);

    // =========================================================================
    // STEP 2: Process each pixel
    // =========================================================================
    for (const auto& pixel : shape.pixels) {
        // Get original color for alpha
        auto origIt = m_originalColors.find(pixel);
        if (origIt == m_originalColors.end()) {
            continue;
        }
        doc::color_t origColor = origIt->second;
        int origAlpha = doc::rgba_geta(origColor);

        // Handle transparency
        if (origAlpha == 0 && config.fillMode != FillMode::BoundedArea &&
            config.fillMode != FillMode::AllNonTransparent) {
            continue;
        }
        int outputAlpha = (origAlpha == 0) ? 255 : origAlpha;

        // -----------------------------------------------------------------
        // STEP 2a: Calculate 3D normal based on ShapeType
        // -----------------------------------------------------------------

        // Calculate radial direction from center to pixel
        double dx = pixel.x - shape.centerX;
        double dy = pixel.y - shape.centerY;
        double distFromCenter = std::sqrt(dx * dx + dy * dy);

        // Get distance from edge (used for Adaptive shading and reflected light)
        float distFromEdge = shape.getDistance(pixel);

        // Normalize to get radial direction (outward from center)
        double dirX = 0, dirY = -1;  // Default: up
        if (distFromCenter > 0.001) {
            dirX = dx / distFromCenter;
            dirY = dy / distFromCenter;
        }

        double normalX = 0, normalY = 0, normalZ = 1;

        switch (config.shapeType) {
            case ShapeType::Sphere: {
                // SPHERE: Perfect spherical normals using radial distance from center
                // This creates smooth, perfectly round shading regardless of actual shape
                // Uses the bounding box diagonal as the "radius" for normalization
                double boundsRadius = std::max(shape.bounds.w, shape.bounds.h) / 2.0;
                if (boundsRadius < 1.0) boundsRadius = maxRadius;

                // Normalized distance from center (0 at center, 1 at edge of bounding circle)
                double normDist = distFromCenter / boundsRadius;
                if (normDist > 1.0) normDist = 1.0;

                // True sphere formula: height = sqrt(1 - r²)
                // At center (r=0): height=1, at edge (r=1): height=0
                double sphereHeight = std::sqrt(std::max(0.0, 1.0 - normDist * normDist));

                // Apply roundness to adjust curvature
                double curvedHeight = sphereHeight;
                if (config.roundness != 1.0 && config.roundness > 0.001) {
                    curvedHeight = std::pow(sphereHeight, 1.0 / config.roundness);
                }

                normalZ = curvedHeight;
                double radialScale = std::sqrt(std::max(0.0, 1.0 - normalZ * normalZ));
                normalX = dirX * radialScale;
                normalY = dirY * radialScale;
                break;
            }

            case ShapeType::Adaptive: {
                // ADAPTIVE: Follow actual shape silhouette using distance map
                // Creates normals that respect the shape's actual edges
                double normHeight = distFromEdge / maxRadius;
                if (normHeight > 1.0) normHeight = 1.0;

                double curvedHeight = normHeight;
                if (config.roundness > 0.001) {
                    curvedHeight = std::pow(normHeight, 1.0 / config.roundness);
                }

                normalZ = curvedHeight;
                double radialScale = std::sqrt(std::max(0.0, 1.0 - normalZ * normalZ));
                normalX = dirX * radialScale;
                normalY = dirY * radialScale;
                break;
            }

            case ShapeType::Cylinder: {
                // CYLINDER: Normals vary only horizontally (vertical axis cylinder)
                // Good for arms, legs, trunks, poles
                double halfWidth = shape.bounds.w / 2.0;
                if (halfWidth < 1.0) halfWidth = 1.0;

                // Normalized horizontal position (-1 to 1)
                double normX = dx / halfWidth;
                if (normX > 1.0) normX = 1.0;
                if (normX < -1.0) normX = -1.0;

                // Cylinder formula: Z = sqrt(1 - x²), normal points outward in X
                normalZ = std::sqrt(std::max(0.0, 1.0 - normX * normX));
                normalX = normX;
                normalY = 0;  // No Y component for vertical cylinder

                // Apply roundness
                if (config.roundness != 1.0 && config.roundness > 0.001) {
                    normalZ = std::pow(normalZ, 1.0 / config.roundness);
                    double len = std::sqrt(normalX * normalX + normalZ * normalZ);
                    if (len > 0.001) {
                        normalX /= len;
                        normalZ /= len;
                    }
                }
                break;
            }

            case ShapeType::Flat: {
                // FLAT: All normals point straight toward viewer
                // Creates uniform lighting across the entire shape
                normalX = 0;
                normalY = 0;
                normalZ = 1;
                break;
            }
        }

        // -----------------------------------------------------------------
        // STEP 2b: Calculate lighting intensity using 3D dot product
        // dot(normal, light) = how much the surface faces the light
        // -----------------------------------------------------------------
        // 3D dot product: normalXY dot lightXY + normalZ * lightZ
        double dotProduct = normalX * lightX + normalY * lightY + normalZ * lightZ;

        // Convert to intensity: ambient + diffuse
        // Diffuse only contributes when dot > 0 (facing light)
        double diffuse = std::max(0.0, dotProduct);
        double intensity = config.ambientLevel + (1.0 - config.ambientLevel) * diffuse;

        // -----------------------------------------------------------------
        // STEP 2b2: Apply highlight position offset
        // The user-controlled highlight point shifts where the brightest area appears
        // The highlight position is independent of light angle (freely movable)
        // Skip for ClassicCartoon - it needs pure, clean intensity bands
        // -----------------------------------------------------------------
        if (config.shadingStyle != ShadingStyle::ClassicCartoon) {
            // Calculate highlight target position using independent highlight angle
            double highlightAngleRad = m_highlightAngle * (PI / 180.0);
            double highlightTargetDist = maxRadius * m_highlightDistanceRatio;
            double highlightTargetX = shape.centerX + std::cos(highlightAngleRad) * highlightTargetDist;
            double highlightTargetY = shape.centerY - std::sin(highlightAngleRad) * highlightTargetDist;

            // Calculate distance from this pixel to the highlight target
            double toHighlightX = pixel.x - highlightTargetX;
            double toHighlightY = pixel.y - highlightTargetY;
            double distToHighlight = std::sqrt(toHighlightX * toHighlightX + toHighlightY * toHighlightY);

            // Create a position boost based on proximity to target
            // Uses Gaussian falloff: closer to target = more boost
            double positionRadius = maxRadius * 0.5;  // Influence radius
            double positionBoost = std::exp(-(distToHighlight * distToHighlight) / (2.0 * positionRadius * positionRadius));

            // Blend the position boost with the lighting intensity
            // This shifts the brightest point toward the user-specified position
            double positionInfluence = 0.4;  // How much the highlight position affects shading
            intensity = intensity * (1.0 - positionInfluence) +
                       (intensity + positionBoost * (1.0 - config.ambientLevel)) * positionInfluence;
        }

        // Clamp intensity to valid range
        intensity = std::min(1.0, std::max(0.0, intensity));

        // -----------------------------------------------------------------
        // STEP 2c: Check for reflected light on shadow-side edges
        // "Never let shadow extend all the way to the edge"
        // Skip for ClassicCartoon - it needs pure, clean bands without extra effects
        // -----------------------------------------------------------------
        // distFromEdge already calculated above for normal projection
        bool useReflectedLight = false;

        if (config.enableReflectedLight &&
            config.shadingStyle != ShadingStyle::ClassicCartoon) {
            // Shadow region: intensity below threshold
            bool isInShadow = intensity < 0.4;
            // Near edge: within reflected light width
            int reflectedWidth = (shape.maxDistance < 5) ? 1 : config.reflectedLightWidth;
            bool isNearEdge = distFromEdge < reflectedWidth;

            useReflectedLight = isInShadow && isNearEdge;
        }

        // -----------------------------------------------------------------
        // STEP 2d: Map intensity to color with pixel-perfect band boundaries
        // -----------------------------------------------------------------
        int r, g, b;

        // Apply pixel-perfect line snapping for band boundaries
        // Uses Bresenham-style quantization to create clean diagonal lines
        // Skip for ClassicCartoon - use pure intensity for cleanest bands
        double adjustedIntensity = intensity;

        if ((config.shadingMode == ShadingMode::ThreeShade ||
             config.shadingMode == ShadingMode::FiveShade) &&
            config.shadingStyle != ShadingStyle::ClassicCartoon) {
            // Project pixel position along the light direction axis
            double lightDist = dx * lightX + dy * lightY;

            // Calculate perpendicular distance (bands run perpendicular to light)
            double perpX = -lightY;
            double perpY = lightX;
            double perpDist = dx * perpX + dy * perpY;

            // Bresenham-style stepping for pixel-perfect diagonal boundaries
            // Key insight: use perpendicular row index to determine stepping pattern
            double absLightX = std::abs(lightX);
            double absLightY = std::abs(lightY);

            // Slope determines the stepping pattern (0 = axis-aligned, 1 = 45 degrees)
            double slope = std::min(absLightX, absLightY) /
                          (std::max(absLightX, absLightY) + 0.001);

            // Row index along the perpendicular axis
            int perpRow = static_cast<int>(std::floor(perpDist));

            // Bresenham error accumulation: fractional part of (row * slope)
            // This determines when the boundary should "step" by one pixel
            double accumError = std::abs(perpRow) * slope;
            double fractionalError = accumError - std::floor(accumError);

            // Calculate step width based on shape size (approx pixels per band)
            double stepWidth = maxRadius * 0.12;
            if (stepWidth < 1.5) stepWidth = 1.5;

            // Quantize light distance to band index
            double shiftedLightDist = lightDist + maxRadius; // Shift to positive range
            int bandIndex = static_cast<int>(std::floor(shiftedLightDist / stepWidth));

            // Apply Bresenham stepping: shift band boundary based on error
            // When fractional error crosses threshold, step the boundary
            if (fractionalError > 0.5) {
                // Step the boundary by adjusting which band this pixel belongs to
                bandIndex += (perpRow % 2 == 0) ? 1 : -1;
            }

            // Convert quantized band back to intensity
            // This snaps the band boundary to pixel-perfect diagonal lines
            double quantizedDist = (bandIndex * stepWidth) - maxRadius;
            double normalizedQuantized = quantizedDist / (maxRadius + 0.001);

            // Blend quantized with original intensity for smooth but clean boundaries
            // Higher blend = sharper stepping, lower = softer
            double blendFactor = 0.35;
            double quantizedIntensity = 0.5 + normalizedQuantized * 0.4;
            adjustedIntensity = intensity * (1.0 - blendFactor) +
                               quantizedIntensity * blendFactor;
        }

        // -----------------------------------------------------------------
        // STEP 2e: Apply highlight focus - make highlight more concentrated
        // Higher focus = higher threshold needed for highlight
        // -----------------------------------------------------------------
        // Calculate dynamic thresholds based on highlight focus
        // focus 0.0 = default thresholds, focus 1.0 = very concentrated highlight
        double highlightBoost = config.highlightFocus * 0.25;  // Max 0.25 boost
        double threeShadeHighThreshold = 0.67 + highlightBoost;
        double fiveShadeHighThreshold = 0.8 + (highlightBoost * 0.6);
        double fiveShadeMidHighThreshold = 0.6 + (highlightBoost * 0.4);

        // Dithering zone width (in intensity units)
        double ditherZone = config.enableDithering ? (0.03 * config.ditheringWidth) : 0.0;

        // Checkerboard pattern for dithering based on pixel position
        bool ditherPattern = ((pixel.x + pixel.y) % 2) == 0;

        // Helper lambda to check if we're in a dithering zone and should use alternate color
        auto shouldDitherToNext = [&](double threshold) -> bool {
            if (!config.enableDithering) return false;
            double distFromThreshold = adjustedIntensity - threshold;
            // In the zone just below threshold
            if (distFromThreshold > -ditherZone && distFromThreshold < 0) {
                return ditherPattern;
            }
            return false;
        };

        auto shouldDitherToPrev = [&](double threshold) -> bool {
            if (!config.enableDithering) return false;
            double distFromThreshold = adjustedIntensity - threshold;
            // In the zone just above threshold
            if (distFromThreshold >= 0 && distFromThreshold < ditherZone) {
                return !ditherPattern;
            }
            return false;
        };

        if (useReflectedLight) {
            // Reflected light: use base color (mid-tone)
            r = doc::rgba_getr(config.baseColor);
            g = doc::rgba_getg(config.baseColor);
            b = doc::rgba_getb(config.baseColor);
        }
        else {
            // Map intensity to shade colors based on mode
            switch (config.shadingMode) {
                case ShadingMode::ThreeShade:
                    if (adjustedIntensity < 0.33) {
                        // Shadow zone - check if dithering to base
                        if (shouldDitherToNext(0.33)) {
                            r = doc::rgba_getr(config.baseColor);
                            g = doc::rgba_getg(config.baseColor);
                            b = doc::rgba_getb(config.baseColor);
                        } else {
                            r = doc::rgba_getr(config.shadowColor);
                            g = doc::rgba_getg(config.shadowColor);
                            b = doc::rgba_getb(config.shadowColor);
                        }
                    }
                    else if (adjustedIntensity < threeShadeHighThreshold) {
                        // Base zone - check if dithering to shadow or highlight
                        if (shouldDitherToPrev(0.33)) {
                            r = doc::rgba_getr(config.shadowColor);
                            g = doc::rgba_getg(config.shadowColor);
                            b = doc::rgba_getb(config.shadowColor);
                        } else if (shouldDitherToNext(threeShadeHighThreshold)) {
                            r = doc::rgba_getr(config.highlightColor);
                            g = doc::rgba_getg(config.highlightColor);
                            b = doc::rgba_getb(config.highlightColor);
                        } else {
                            r = doc::rgba_getr(config.baseColor);
                            g = doc::rgba_getg(config.baseColor);
                            b = doc::rgba_getb(config.baseColor);
                        }
                    }
                    else {
                        // Highlight zone - check if dithering to base
                        if (shouldDitherToPrev(threeShadeHighThreshold)) {
                            r = doc::rgba_getr(config.baseColor);
                            g = doc::rgba_getg(config.baseColor);
                            b = doc::rgba_getb(config.baseColor);
                        } else {
                            r = doc::rgba_getr(config.highlightColor);
                            g = doc::rgba_getg(config.highlightColor);
                            b = doc::rgba_getb(config.highlightColor);
                        }
                    }
                    break;

                case ShadingMode::FiveShade:
                    if (adjustedIntensity < 0.2) {
                        if (shouldDitherToNext(0.2)) {
                            r = doc::rgba_getr(config.midShadowColor);
                            g = doc::rgba_getg(config.midShadowColor);
                            b = doc::rgba_getb(config.midShadowColor);
                        } else {
                            r = doc::rgba_getr(config.shadowColor);
                            g = doc::rgba_getg(config.shadowColor);
                            b = doc::rgba_getb(config.shadowColor);
                        }
                    }
                    else if (adjustedIntensity < 0.4) {
                        if (shouldDitherToPrev(0.2)) {
                            r = doc::rgba_getr(config.shadowColor);
                            g = doc::rgba_getg(config.shadowColor);
                            b = doc::rgba_getb(config.shadowColor);
                        } else if (shouldDitherToNext(0.4)) {
                            r = doc::rgba_getr(config.baseColor);
                            g = doc::rgba_getg(config.baseColor);
                            b = doc::rgba_getb(config.baseColor);
                        } else {
                            r = doc::rgba_getr(config.midShadowColor);
                            g = doc::rgba_getg(config.midShadowColor);
                            b = doc::rgba_getb(config.midShadowColor);
                        }
                    }
                    else if (adjustedIntensity < fiveShadeMidHighThreshold) {
                        if (shouldDitherToPrev(0.4)) {
                            r = doc::rgba_getr(config.midShadowColor);
                            g = doc::rgba_getg(config.midShadowColor);
                            b = doc::rgba_getb(config.midShadowColor);
                        } else if (shouldDitherToNext(fiveShadeMidHighThreshold)) {
                            r = doc::rgba_getr(config.midHighlightColor);
                            g = doc::rgba_getg(config.midHighlightColor);
                            b = doc::rgba_getb(config.midHighlightColor);
                        } else {
                            r = doc::rgba_getr(config.baseColor);
                            g = doc::rgba_getg(config.baseColor);
                            b = doc::rgba_getb(config.baseColor);
                        }
                    }
                    else if (adjustedIntensity < fiveShadeHighThreshold) {
                        if (shouldDitherToPrev(fiveShadeMidHighThreshold)) {
                            r = doc::rgba_getr(config.baseColor);
                            g = doc::rgba_getg(config.baseColor);
                            b = doc::rgba_getb(config.baseColor);
                        } else if (shouldDitherToNext(fiveShadeHighThreshold)) {
                            r = doc::rgba_getr(config.highlightColor);
                            g = doc::rgba_getg(config.highlightColor);
                            b = doc::rgba_getb(config.highlightColor);
                        } else {
                            r = doc::rgba_getr(config.midHighlightColor);
                            g = doc::rgba_getg(config.midHighlightColor);
                            b = doc::rgba_getb(config.midHighlightColor);
                        }
                    }
                    else {
                        if (shouldDitherToPrev(fiveShadeHighThreshold)) {
                            r = doc::rgba_getr(config.midHighlightColor);
                            g = doc::rgba_getg(config.midHighlightColor);
                            b = doc::rgba_getb(config.midHighlightColor);
                        } else {
                            r = doc::rgba_getr(config.highlightColor);
                            g = doc::rgba_getg(config.highlightColor);
                            b = doc::rgba_getb(config.highlightColor);
                        }
                    }
                    break;

                case ShadingMode::Gradient:
                default:
                    // Smooth gradient between shadow -> base -> highlight
                    if (intensity < 0.5) {
                        double t = intensity * 2.0;
                        r = static_cast<int>(doc::rgba_getr(config.shadowColor) * (1.0 - t) +
                                             doc::rgba_getr(config.baseColor) * t);
                        g = static_cast<int>(doc::rgba_getg(config.shadowColor) * (1.0 - t) +
                                             doc::rgba_getg(config.baseColor) * t);
                        b = static_cast<int>(doc::rgba_getb(config.shadowColor) * (1.0 - t) +
                                             doc::rgba_getb(config.baseColor) * t);
                    }
                    else {
                        double t = (intensity - 0.5) * 2.0;
                        r = static_cast<int>(doc::rgba_getr(config.baseColor) * (1.0 - t) +
                                             doc::rgba_getr(config.highlightColor) * t);
                        g = static_cast<int>(doc::rgba_getg(config.baseColor) * (1.0 - t) +
                                             doc::rgba_getg(config.highlightColor) * t);
                        b = static_cast<int>(doc::rgba_getb(config.baseColor) * (1.0 - t) +
                                             doc::rgba_getb(config.highlightColor) * t);
                    }
                    break;
            }
        }

        // -----------------------------------------------------------------
        // STEP 2f: Apply shading style effects
        // These modify the color placement/pattern based on selected style
        // -----------------------------------------------------------------
        bool skipPixel = false;  // For pattern styles that skip certain pixels

        switch (config.shadingStyle) {
            case ShadingStyle::ClassicCartoon:
                // Clean solid color zones with hard edges - no extra effects
                // This is the simplest, cleanest shading style
                break;

            case ShadingStyle::SoftCartoon: {
                // Add soft transition at band boundaries using dithering
                // Creates softer appearance while keeping limited palette
                double bandWidth = 0.08;  // Transition zone width
                bool nearBoundary = false;
                double boundaryDist = 0;

                // Check distance from band boundaries
                if (config.shadingMode == ShadingMode::ThreeShade) {
                    double dist1 = std::abs(adjustedIntensity - 0.33);
                    double dist2 = std::abs(adjustedIntensity - threeShadeHighThreshold);
                    boundaryDist = std::min(dist1, dist2);
                    nearBoundary = boundaryDist < bandWidth;
                }
                else if (config.shadingMode == ShadingMode::FiveShade) {
                    double dist1 = std::abs(adjustedIntensity - 0.2);
                    double dist2 = std::abs(adjustedIntensity - 0.4);
                    double dist3 = std::abs(adjustedIntensity - fiveShadeMidHighThreshold);
                    double dist4 = std::abs(adjustedIntensity - fiveShadeHighThreshold);
                    boundaryDist = std::min({dist1, dist2, dist3, dist4});
                    nearBoundary = boundaryDist < bandWidth;
                }

                // Apply soft dithering near boundaries
                if (nearBoundary) {
                    double ditherProb = 1.0 - (boundaryDist / bandWidth);
                    // Use position-based pseudo-random for consistent pattern
                    unsigned int seed = static_cast<unsigned int>(pixel.x * 73856093 ^ pixel.y * 19349663);
                    double randVal = (seed % 1000) / 1000.0;
                    if (randVal < ditherProb * 0.3) {
                        // Slight color variation at boundaries
                        int variation = static_cast<int>((randVal - 0.15) * 30);
                        r = std::max(0, std::min(255, r + variation));
                        g = std::max(0, std::min(255, g + variation));
                        b = std::max(0, std::min(255, b + variation));
                    }
                }
                break;
            }

            case ShadingStyle::OilSoft: {
                // Painterly, irregular texture using Perlin-like noise
                // Add controlled randomness for organic feeling
                double noiseScale = 0.15;
                unsigned int seed = static_cast<unsigned int>(
                    pixel.x * 12345 + pixel.y * 67890 +
                    static_cast<int>(pixel.x * 0.1) * 111 +
                    static_cast<int>(pixel.y * 0.1) * 222);
                double noise = ((seed % 1000) / 1000.0 - 0.5) * 2.0;

                // Vary intensity based on noise
                int variation = static_cast<int>(noise * noiseScale * 60 * (1.0 - intensity * 0.5));
                r = std::max(0, std::min(255, r + variation));
                g = std::max(0, std::min(255, g + variation));
                b = std::max(0, std::min(255, b + variation));
                break;
            }

            case ShadingStyle::RawPaint: {
                // Smooth dithered gradients using ordered dithering
                // Bayer 2x2 matrix pattern
                int bayerMatrix[2][2] = {{0, 2}, {3, 1}};
                int bx = pixel.x % 2;
                int by = pixel.y % 2;
                double threshold = (bayerMatrix[by][bx] + 0.5) / 4.0;

                // Apply dithering between adjacent colors for smoother gradients
                double fractIntensity = adjustedIntensity * 3.0;  // Scale to color bands
                double fract = fractIntensity - std::floor(fractIntensity);

                if (fract > threshold && adjustedIntensity < 0.9) {
                    // Bump to next lighter color occasionally
                    int bump = 15;
                    r = std::min(255, r + bump);
                    g = std::min(255, g + bump);
                    b = std::min(255, b + bump);
                }
                break;
            }

            case ShadingStyle::Dotted: {
                // Screen-tone dot pattern - dots denser in shadows
                int gridSize = 3;  // Dot grid spacing
                int gx = pixel.x % gridSize;
                int gy = pixel.y % gridSize;

                // Dot size varies with intensity (bigger dots in shadows)
                double dotRadius = (1.0 - adjustedIntensity) * 1.5;
                double centerDist = std::sqrt((gx - gridSize/2.0) * (gx - gridSize/2.0) +
                                              (gy - gridSize/2.0) * (gy - gridSize/2.0));

                if (centerDist <= dotRadius && adjustedIntensity < 0.8) {
                    // Inside a dot - use shadow color
                    r = doc::rgba_getr(config.shadowColor);
                    g = doc::rgba_getg(config.shadowColor);
                    b = doc::rgba_getb(config.shadowColor);
                }
                break;
            }

            case ShadingStyle::StrokeSphere: {
                // Concentric strokes - pattern follows shape type
                double strokeDist = 0;
                int ringSpacing = 4;

                switch (config.shapeType) {
                    case ShapeType::Sphere:
                        // Concentric circles from center
                        strokeDist = distFromCenter;
                        break;
                    case ShapeType::Adaptive:
                        // Contour lines following shape edge (inverted distance)
                        strokeDist = maxRadius - distFromEdge;
                        break;
                    case ShapeType::Cylinder:
                        // Horizontal bands for cylinder
                        strokeDist = std::abs(dy);
                        break;
                    case ShapeType::Flat:
                        // Horizontal bands for flat
                        strokeDist = pixel.y - shape.bounds.y;
                        break;
                }

                int ringIndex = static_cast<int>(strokeDist) % ringSpacing;

                // Stroke thickness varies with intensity
                int strokeWidth = static_cast<int>((1.0 - adjustedIntensity) * 2 + 0.5);

                if (ringIndex < strokeWidth && adjustedIntensity < 0.85) {
                    // On a stroke - darken
                    r = static_cast<int>(r * 0.7);
                    g = static_cast<int>(g * 0.7);
                    b = static_cast<int>(b * 0.7);
                }
                break;
            }

            case ShadingStyle::StrokeVertical: {
                // Vertical hatching lines - denser in shadows
                int baseSpacing = 3;
                // Spacing increases with intensity (more lines in shadows)
                int spacing = baseSpacing + static_cast<int>(adjustedIntensity * 3);
                int linePos = pixel.x % spacing;

                // Line thickness varies with intensity
                int lineWidth = (adjustedIntensity < 0.5) ? 2 : 1;

                if (linePos < lineWidth && adjustedIntensity < 0.85) {
                    // On a line - use shadow color
                    r = doc::rgba_getr(config.shadowColor);
                    g = doc::rgba_getg(config.shadowColor);
                    b = doc::rgba_getb(config.shadowColor);
                }
                break;
            }

            case ShadingStyle::StrokeHorizontal: {
                // Horizontal hatching lines - same as vertical but on Y axis
                int baseSpacing = 3;
                int spacing = baseSpacing + static_cast<int>(adjustedIntensity * 3);
                int linePos = pixel.y % spacing;

                int lineWidth = (adjustedIntensity < 0.5) ? 2 : 1;

                if (linePos < lineWidth && adjustedIntensity < 0.85) {
                    r = doc::rgba_getr(config.shadowColor);
                    g = doc::rgba_getg(config.shadowColor);
                    b = doc::rgba_getb(config.shadowColor);
                }
                break;
            }

            case ShadingStyle::SmallGrain: {
                // Fine noise texture - subtle random variation
                unsigned int seed = static_cast<unsigned int>(pixel.x * 73856093 ^ pixel.y * 19349663);
                double noise = ((seed % 1000) / 1000.0 - 0.5);

                // Noise intensity varies with shading (more in shadows)
                double noiseStrength = (1.0 - adjustedIntensity * 0.7) * 25;
                int variation = static_cast<int>(noise * noiseStrength);

                r = std::max(0, std::min(255, r + variation));
                g = std::max(0, std::min(255, g + variation));
                b = std::max(0, std::min(255, b + variation));
                break;
            }

            case ShadingStyle::LargeGrain: {
                // Chunky noise clusters (2x2 or 3x3 pixel groups)
                int clusterSize = 2;
                int cx = (pixel.x / clusterSize) * clusterSize;
                int cy = (pixel.y / clusterSize) * clusterSize;

                unsigned int seed = static_cast<unsigned int>(cx * 73856093 ^ cy * 19349663);
                double noise = ((seed % 1000) / 1000.0 - 0.5);

                double noiseStrength = (1.0 - adjustedIntensity * 0.6) * 40;
                int variation = static_cast<int>(noise * noiseStrength);

                r = std::max(0, std::min(255, r + variation));
                g = std::max(0, std::min(255, g + variation));
                b = std::max(0, std::min(255, b + variation));
                break;
            }

            case ShadingStyle::TrickyShading: {
                // Complex mixed patterns - combine multiple noise types
                // Layer 1: Large splotches
                int cx = (pixel.x / 4) * 4;
                int cy = (pixel.y / 4) * 4;
                unsigned int seed1 = static_cast<unsigned int>(cx * 12345 ^ cy * 67890);
                double splotch = ((seed1 % 1000) / 1000.0);

                // Layer 2: Fine grain
                unsigned int seed2 = static_cast<unsigned int>(pixel.x * 73856 ^ pixel.y * 19349);
                double grain = ((seed2 % 1000) / 1000.0 - 0.5);

                if (splotch > (0.6 + adjustedIntensity * 0.3)) {
                    // Splotch area - darken
                    r = static_cast<int>(r * 0.8);
                    g = static_cast<int>(g * 0.8);
                    b = static_cast<int>(b * 0.8);
                }

                // Add fine grain on top
                int grainVar = static_cast<int>(grain * 20 * (1.0 - adjustedIntensity * 0.5));
                r = std::max(0, std::min(255, r + grainVar));
                g = std::max(0, std::min(255, g + grainVar));
                b = std::max(0, std::min(255, b + grainVar));
                break;
            }

            case ShadingStyle::SoftPattern: {
                // Gentle Perlin-like noise overlay
                // Use multiple octaves of noise for smoother pattern
                double freq1 = 0.1, freq2 = 0.2;
                unsigned int seed1 = static_cast<unsigned int>(
                    static_cast<int>(pixel.x * freq1) * 12345 +
                    static_cast<int>(pixel.y * freq1) * 67890);
                unsigned int seed2 = static_cast<unsigned int>(
                    static_cast<int>(pixel.x * freq2) * 54321 +
                    static_cast<int>(pixel.y * freq2) * 98765);

                double noise1 = ((seed1 % 1000) / 1000.0 - 0.5);
                double noise2 = ((seed2 % 1000) / 1000.0 - 0.5) * 0.5;
                double combined = (noise1 + noise2) * 0.67;

                double strength = (1.0 - adjustedIntensity * 0.5) * 20;
                int variation = static_cast<int>(combined * strength);

                r = std::max(0, std::min(255, r + variation));
                g = std::max(0, std::min(255, g + variation));
                b = std::max(0, std::min(255, b + variation));
                break;
            }

            case ShadingStyle::Wrinkled: {
                // Flow-based wrinkle lines following surface direction
                // Flow direction depends on shape type
                double flowDist = 0;
                int wrinkleSpacing = 5;

                switch (config.shapeType) {
                    case ShapeType::Sphere: {
                        // Radial wrinkles from center
                        double flowAngle = std::atan2(dy, dx);
                        flowDist = pixel.x * std::cos(flowAngle) + pixel.y * std::sin(flowAngle);
                        break;
                    }
                    case ShapeType::Adaptive: {
                        // Wrinkles follow contour lines (perpendicular to edge)
                        flowDist = distFromEdge * 2.0;
                        break;
                    }
                    case ShapeType::Cylinder: {
                        // Horizontal wrinkles for cylinder
                        flowDist = pixel.y;
                        wrinkleSpacing = 4;
                        break;
                    }
                    case ShapeType::Flat: {
                        // Diagonal wrinkles for flat
                        flowDist = (pixel.x + pixel.y) * 0.7;
                        break;
                    }
                }

                double wrinklePos = std::fmod(std::abs(flowDist), wrinkleSpacing);

                // Wrinkle intensity based on position and shading
                double wrinkleWidth = (1.0 - adjustedIntensity) * 1.5;

                if (wrinklePos < wrinkleWidth && adjustedIntensity < 0.8) {
                    r = static_cast<int>(r * 0.75);
                    g = static_cast<int>(g * 0.75);
                    b = static_cast<int>(b * 0.75);
                }
                break;
            }

            case ShadingStyle::Patterned: {
                // Regular geometric pattern - adapts to shape type
                int patternSize = 6;
                double px, py;
                bool onOutline = false;

                switch (config.shapeType) {
                    case ShapeType::Sphere: {
                        // Scale pattern with perspective distortion toward edges
                        double perspScale = 1.0 + (distFromCenter / maxRadius) * 0.5;
                        px = std::fmod(pixel.x * perspScale, patternSize);
                        py = std::fmod(pixel.y * perspScale, patternSize);
                        // Offset every other row
                        if (static_cast<int>(pixel.y * perspScale / patternSize) % 2 == 1) {
                            px = std::fmod(px + patternSize / 2.0, patternSize);
                        }
                        break;
                    }
                    case ShapeType::Adaptive: {
                        // Pattern follows contour with edge-based distortion
                        double contourScale = 1.0 + (1.0 - distFromEdge / maxRadius) * 0.3;
                        px = std::fmod(pixel.x * contourScale, patternSize);
                        py = std::fmod(pixel.y * contourScale, patternSize);
                        if (static_cast<int>(pixel.y * contourScale / patternSize) % 2 == 1) {
                            px = std::fmod(px + patternSize / 2.0, patternSize);
                        }
                        break;
                    }
                    case ShapeType::Cylinder: {
                        // Stretched pattern horizontally for cylinder
                        px = std::fmod(pixel.x * 0.7, patternSize);
                        py = std::fmod(pixel.y, patternSize);
                        if ((pixel.y / patternSize) % 2 == 1) {
                            px = std::fmod(px + patternSize / 2.0, patternSize);
                        }
                        break;
                    }
                    case ShapeType::Flat:
                    default: {
                        // Regular grid pattern
                        px = pixel.x % patternSize;
                        py = pixel.y % patternSize;
                        if ((pixel.y / patternSize) % 2 == 1) {
                            px = (pixel.x + patternSize / 2) % patternSize;
                        }
                        break;
                    }
                }

                // Distance from pattern cell center
                double pcx = px - patternSize / 2.0;
                double pcy = py - patternSize / 2.0;
                double pDist = std::sqrt(pcx * pcx + pcy * pcy);

                // Create scale/cell outline
                double outlineWidth = 1.0 + (1.0 - adjustedIntensity) * 0.5;
                if (pDist > patternSize / 2.0 - outlineWidth) {
                    // On outline
                    r = static_cast<int>(r * (0.6 + adjustedIntensity * 0.3));
                    g = static_cast<int>(g * (0.6 + adjustedIntensity * 0.3));
                    b = static_cast<int>(b * (0.6 + adjustedIntensity * 0.3));
                }
                break;
            }

            case ShadingStyle::Wood: {
                // Wood grain - pattern follows shape type
                double ringFreq = 0.3;
                double grainDist = 0;

                // Add noise to grain position for organic look
                unsigned int seed = static_cast<unsigned int>(
                    static_cast<int>(pixel.x * 0.1) * 12345 +
                    static_cast<int>(pixel.y * 0.1) * 67890);
                double noise = ((seed % 1000) / 1000.0 - 0.5) * 3;

                switch (config.shapeType) {
                    case ShapeType::Sphere:
                        // Concentric rings from center (cross-section of log)
                        grainDist = distFromCenter;
                        break;
                    case ShapeType::Adaptive:
                        // Rings following shape contour
                        grainDist = maxRadius - distFromEdge;
                        break;
                    case ShapeType::Cylinder:
                        // Vertical grain lines (along the cylinder)
                        grainDist = std::abs(dx) * 1.5;
                        ringFreq = 0.5;
                        break;
                    case ShapeType::Flat:
                        // Horizontal wood grain
                        grainDist = pixel.y - shape.bounds.y;
                        ringFreq = 0.4;
                        break;
                }

                double ringValue = std::sin((grainDist + noise) * ringFreq);

                // Create grain lines
                if (ringValue > 0.3) {
                    // Darker grain line
                    double grainStrength = (ringValue - 0.3) * (1.0 - adjustedIntensity * 0.5);
                    r = static_cast<int>(r * (1.0 - grainStrength * 0.3));
                    g = static_cast<int>(g * (1.0 - grainStrength * 0.3));
                    b = static_cast<int>(b * (1.0 - grainStrength * 0.3));
                }
                break;
            }

            case ShadingStyle::HardBrush: {
                // Visible brush stroke marks following light direction
                double strokeAngle = config.lightAngle * (PI / 180.0);
                double strokeDist = pixel.x * std::cos(strokeAngle) + pixel.y * std::sin(strokeAngle);

                int strokeSpacing = 4;
                int strokePos = static_cast<int>(std::abs(strokeDist)) % strokeSpacing;

                // Stroke width varies with intensity
                int strokeWidth = 1 + static_cast<int>((1.0 - adjustedIntensity) * 1.5);

                // Add slight variation for brush texture
                unsigned int seed = static_cast<unsigned int>(pixel.x * 111 ^ pixel.y * 222);
                int texVar = (seed % 3) - 1;

                if (strokePos < strokeWidth && adjustedIntensity < 0.9) {
                    r = std::max(0, std::min(255, static_cast<int>(r * 0.8) + texVar * 5));
                    g = std::max(0, std::min(255, static_cast<int>(g * 0.8) + texVar * 5));
                    b = std::max(0, std::min(255, static_cast<int>(b * 0.8) + texVar * 5));
                }
                break;
            }
        }

        // Clamp color values
        r = std::max(0, std::min(255, r));
        g = std::max(0, std::min(255, g));
        b = std::max(0, std::min(255, b));

        m_previewColors[pixel] = doc::rgba(r, g, b, outputAlpha);
    }

    // =========================================================================
    // STEP 3: Apply anti-aliasing around edges (optional)
    // Adds semi-transparent pixels outside the shape for smoother silhouette
    // =========================================================================
    if (config.enableAntiAliasing && !shape.edgePixels.empty()) {
        // Direction offsets for neighbor checking
        const int dx8[] = {-1, 0, 1, -1, 1, -1, 0, 1};
        const int dy8[] = {-1, -1, -1, 0, 0, 1, 1, 1};

        // Calculate light direction for AA color selection
        double angleRad = config.lightAngle * (3.14159265358979323846 / 180.0);
        double aaLightX = std::cos(angleRad);
        double aaLightY = -std::sin(angleRad);

        for (const auto& edge : shape.edgePixels) {
            // Get the shaded color of this edge pixel
            auto edgeColorIt = m_previewColors.find(edge);
            if (edgeColorIt == m_previewColors.end()) continue;

            doc::color_t edgeColor = edgeColorIt->second;

            // Check all 8 neighbors
            for (int i = 0; i < 8; ++i) {
                gfx::Point neighbor(edge.x + dx8[i], edge.y + dy8[i]);

                // Skip if neighbor is inside the shape
                if (shape.contains(neighbor)) continue;

                // Skip if we already have an AA pixel here
                if (m_previewColors.find(neighbor) != m_previewColors.end()) continue;

                // Calculate direction from center to this AA pixel
                double ndx = neighbor.x - shape.centerX;
                double ndy = neighbor.y - shape.centerY;
                double nlen = std::sqrt(ndx * ndx + ndy * ndy);
                if (nlen > 0.001) {
                    ndx /= nlen;
                    ndy /= nlen;
                }

                // Determine AA alpha based on position (corners get lower alpha)
                // Cardinal directions (0, 2, 5, 7) = stronger AA
                // Diagonal directions (1, 3, 4, 6) = weaker AA
                int baseAlpha = (i == 1 || i == 3 || i == 4 || i == 6) ? 96 : 128;

                // Adjust alpha based on AA levels setting
                baseAlpha = baseAlpha * config.aaLevels / 2;
                if (baseAlpha > 200) baseAlpha = 200;

                // Blend outline color based on light direction for selective outline AA
                doc::color_t aaColor;
                if (config.enableSelectiveOutline) {
                    double dotLight = ndx * aaLightX + ndy * aaLightY;
                    if (dotLight > 0) {
                        // Light side - use lighter outline
                        aaColor = config.lightOutlineColor;
                    } else {
                        // Shadow side - use darker outline
                        aaColor = config.shadowOutlineColor;
                    }
                } else {
                    // Default: blend edge color with transparency
                    aaColor = edgeColor;
                }

                // Create AA pixel with calculated alpha
                m_previewColors[neighbor] = doc::rgba(
                    doc::rgba_getr(aaColor),
                    doc::rgba_getg(aaColor),
                    doc::rgba_getb(aaColor),
                    baseAlpha);
            }
        }
    }

    SHADE_LOG("generateShadedPreview: generated %zu colors (with AA)", m_previewColors.size());
}

void AutoShadeState::applyShading(Editor* editor)
{
    if (!m_hasPreview || m_previewColors.empty()) {
        return;
    }

    Site site = editor->getSite();
    Doc* doc = site.document();
    Sprite* sprite = site.sprite();
    Layer* layer = site.layer();
    Cel* cel = site.cel();

    if (!doc || !sprite || !layer || !cel) {
        return;
    }

    // Create transaction for undo support
    Tx tx(doc, "Auto-Shade");

    // Use ExpandCelCanvas to handle cel bounds expansion if needed
    ExpandCelCanvas expand(
        site, layer,
        TiledMode::NONE,
        tx,
        ExpandCelCanvas::None);

    // Validate destination canvas with the shape bounds
    gfx::Rect shapeBounds = m_tool.lastShape().bounds;
    gfx::Region rgn(shapeBounds);
    expand.validateDestCanvas(rgn);

    doc::Image* dstImage = expand.getDestCanvas();
    doc::PixelFormat format = sprite->pixelFormat();

    // Get palette for indexed images
    const doc::Palette* palette = nullptr;
    if (format == doc::IMAGE_INDEXED) {
        palette = sprite->palette(site.frame());
    }

    // Get cel origin for coordinate conversion
    gfx::Point celOrigin = expand.getCelOrigin();

    // Apply each preview pixel
    for (const auto& pair : m_previewColors) {
        const gfx::Point& pos = pair.first;
        doc::color_t color = pair.second;

        // Convert sprite coordinates to image coordinates
        int x = pos.x - celOrigin.x;
        int y = pos.y - celOrigin.y;

        if (x >= 0 && y >= 0 && x < dstImage->width() && y < dstImage->height()) {
            // Convert color to destination pixel format
            doc::color_t dstColor = color;

            switch (format) {
                case doc::IMAGE_RGB:
                    // Already in RGB format
                    dstColor = color;
                    break;

                case doc::IMAGE_INDEXED:
                    // Convert RGB to palette index
                    if (palette) {
                        dstColor = palette->findBestfit(
                            doc::rgba_getr(color),
                            doc::rgba_getg(color),
                            doc::rgba_getb(color),
                            doc::rgba_geta(color),
                            0);  // mask index
                    }
                    break;

                case doc::IMAGE_GRAYSCALE:
                    // Convert RGB to grayscale
                    {
                        int gray = (doc::rgba_getr(color) * 30 +
                                    doc::rgba_getg(color) * 59 +
                                    doc::rgba_getb(color) * 11) / 100;
                        dstColor = doc::graya(gray, doc::rgba_geta(color));
                    }
                    break;

                default:
                    break;
            }

            doc::put_pixel(dstImage, x, y, dstColor);
        }
    }

    // Commit the changes
    expand.commit();
    tx.commit();

    // Clear preview
    cancelPreview(editor);

    // Invalidate document
    doc->notifyGeneralUpdate();
}

void AutoShadeState::cancelPreview(Editor* editor)
{
    m_hasPreview = false;
    m_previewColors.clear();
    m_originalColors.clear();
    m_tool.reset();

    invalidateEditor(editor);
}

//------------------------------------------------------------------------------
// Actions
//------------------------------------------------------------------------------

void AutoShadeState::analyzeRegion(Editor* editor, const gfx::Point& spritePos)
{
    Site site = editor->getSite();
    Doc* doc = site.document();
    Sprite* sprite = site.sprite();
    Cel* cel = site.cel();

    if (!doc || !sprite || !cel) {
        return;
    }

    doc::Image* image = cel->image();
    if (!image) {
        return;
    }

    // Adjust position for cel offset
    int imgX = spritePos.x - cel->x();
    int imgY = spritePos.y - cel->y();

    // Check bounds
    if (imgX < 0 || imgY < 0 || imgX >= image->width() || imgY >= image->height()) {
        return;
    }

    // Get palette for indexed images (for proper color comparison in flood fill)
    const doc::Palette* palette = nullptr;
    if (image->pixelFormat() == doc::IMAGE_INDEXED) {
        palette = sprite->palette(site.frame());
    }

    // Analyze the region at this position
    if (m_tool.analyzeRegion(image, imgX, imgY, palette)) {
        ShapeData& shape = m_tool.lastShape();
        SHADE_LOG("analyzeRegion: pixels=%d outline=%d (edge pixels) maxDist=%.1f center=(%.1f,%.1f)",
                  shape.pixelCount(), (int)shape.edgePixels.size(), shape.maxDistance,
                  shape.centerX, shape.centerY);

        // Capture original colors BEFORE adjusting coordinates (still in image coords)
        m_originalColors.clear();
        for (const auto& pixel : shape.pixels) {
            doc::color_t origColor = doc::get_pixel(image, pixel.x, pixel.y);
            // Store with sprite coordinates
            gfx::Point spritePixel(pixel.x + cel->x(), pixel.y + cel->y());
            m_originalColors[spritePixel] = origColor;
        }

        // Adjust shape positions back to sprite coordinates
        for (auto& pixel : shape.pixels) {
            pixel.x += cel->x();
            pixel.y += cel->y();
        }
        for (auto& edge : shape.edgePixels) {
            edge.x += cel->x();
            edge.y += cel->y();
        }
        // Rebuild pixel set with adjusted coordinates
        shape.pixelSet.clear();
        for (const auto& pixel : shape.pixels) {
            shape.pixelSet.insert(pixel);
        }
        // Rebuild distance map with adjusted coordinates
        tools::DistanceMap adjustedDistanceMap;
        for (const auto& entry : shape.distanceMap) {
            gfx::Point adjustedPos(entry.first.x + cel->x(), entry.first.y + cel->y());
            adjustedDistanceMap[adjustedPos] = entry.second;
        }
        shape.distanceMap = std::move(adjustedDistanceMap);
        // Adjust bounds
        shape.bounds.x += cel->x();
        shape.bounds.y += cel->y();
        // Adjust center
        shape.centerX += cel->x();
        shape.centerY += cel->y();

        // Reset highlight position to match current light direction
        m_highlightAngle = m_tool.config().lightAngle;
        m_highlightDistanceRatio = 0.3;  // Default: 30% from center

        // Generate preview using original colors
        generateShadedPreview();
        m_hasPreview = true;

        invalidateEditor(editor);
    }
}

//------------------------------------------------------------------------------
// Drawing helpers
//------------------------------------------------------------------------------

void AutoShadeState::invalidateEditor(Editor* editor)
{
    if (m_hasPreview && !m_tool.lastShape().isEmpty()) {
        // Invalidate the shape bounds plus light handle area
        gfx::Rect bounds = m_tool.lastShape().bounds;
        // Expand to include light handle (1.5x max radius + handle size)
        int handleMargin = static_cast<int>(m_tool.lastShape().maxDistance * 0.6) + 10;
        bounds.enlarge(handleMargin);
        editor->invalidateRect(editor->editorToScreen(bounds));
    }
    else {
        // Invalidate entire editor
        editor->invalidate();
    }
}

//------------------------------------------------------------------------------
// Light handle helpers
//------------------------------------------------------------------------------

gfx::Point AutoShadeState::getLightHandlePosition() const
{
    if (m_tool.lastShape().isEmpty()) {
        return gfx::Point(0, 0);
    }

    const ShapeData& shape = m_tool.lastShape();
    const ShadeConfig& config = m_tool.config();

    const double PI = 3.14159265358979323846;
    double angleRad = config.lightAngle * (PI / 180.0);

    // Calculate sun handle distance based on elevation
    // elevation 0° = at edge (minDist), elevation 90° = far away (maxDist)
    double minDist = shape.maxDistance;
    double maxDist = shape.maxDistance * 3.0;
    double elevationFactor = config.lightElevation / 90.0;
    if (elevationFactor > 1.0) elevationFactor = 1.0;
    double handleDist = minDist + (maxDist - minDist) * elevationFactor;

    // Light source position (where light comes FROM)
    int handleX = static_cast<int>(shape.centerX + std::cos(angleRad) * handleDist);
    int handleY = static_cast<int>(shape.centerY - std::sin(angleRad) * handleDist);

    return gfx::Point(handleX, handleY);
}

bool AutoShadeState::hitTestLightHandle(Editor* editor, const gfx::Point& screenPos) const
{
    if (!m_hasPreview || m_tool.lastShape().isEmpty()) {
        return false;
    }

    // Get handle position in sprite coordinates
    gfx::Point handlePos = getLightHandlePosition();

    // Convert handle position to screen coordinates
    gfx::Point handleScreen = editor->editorToScreen(handlePos);

    // Hit test with a generous radius (10 pixels in screen space)
    int dx = screenPos.x - handleScreen.x;
    int dy = screenPos.y - handleScreen.y;
    int hitRadius = 10;

    return (dx * dx + dy * dy) <= (hitRadius * hitRadius);
}

//------------------------------------------------------------------------------
// Highlight point helpers (incidence point - where light hits the shape)
//------------------------------------------------------------------------------

gfx::Point AutoShadeState::getHighlightPosition() const
{
    if (m_tool.lastShape().isEmpty()) {
        return gfx::Point(0, 0);
    }

    const ShapeData& shape = m_tool.lastShape();

    const double PI = 3.14159265358979323846;
    // Use independent highlight angle (not tied to light angle)
    double angleRad = m_highlightAngle * (PI / 180.0);

    // Highlight point is at m_highlightDistanceRatio of maxDistance from center
    double highlightDist = shape.maxDistance * m_highlightDistanceRatio;
    int highlightX = static_cast<int>(shape.centerX + std::cos(angleRad) * highlightDist);
    int highlightY = static_cast<int>(shape.centerY - std::sin(angleRad) * highlightDist);

    return gfx::Point(highlightX, highlightY);
}

bool AutoShadeState::hitTestHighlightPoint(Editor* editor, const gfx::Point& screenPos) const
{
    if (!m_hasPreview || m_tool.lastShape().isEmpty()) {
        return false;
    }

    // Get highlight position in sprite coordinates
    gfx::Point highlightPos = getHighlightPosition();

    // Convert to screen coordinates
    gfx::Point highlightScreen = editor->editorToScreen(highlightPos);

    // Hit test with a generous radius (8 pixels in screen space)
    int dx = screenPos.x - highlightScreen.x;
    int dy = screenPos.y - highlightScreen.y;
    int hitRadius = 8;

    return (dx * dx + dy * dy) <= (hitRadius * hitRadius);
}

} // namespace app
