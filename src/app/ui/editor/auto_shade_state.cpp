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
    }

    void getInvalidDecoratoredRegion(Editor* editor, gfx::Region& region) override {
        if (!m_state->m_hasPreview || m_state->m_tool.lastShape().isEmpty()) {
            return;
        }

        // Add the shape bounds to the invalidated region
        const ShapeData& shape = m_state->m_tool.lastShape();
        gfx::Rect bounds = shape.bounds;

        // Expand slightly to include outline
        bounds.enlarge(1);

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

    // Load colors from color bar
    ColorBar* colorBar = ColorBar::instance();
    if (colorBar) {
        app::Color fg = colorBar->getFgColor();
        // Use foreground as base color (always opaque for shading)
        int r = fg.getRed();
        int g = fg.getGreen();
        int b = fg.getBlue();

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

    // Update status bar with position
    editor->updateStatusBar();

    return true;
}

bool AutoShadeState::onMouseUp(Editor* editor, ui::MouseMessage* msg)
{
    if (msg->left() && m_mouseDown) {
        m_mouseDown = false;
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
            "Auto-Shade: Light angle: {:.0f}° | Ambient: {:.0f}% | Press Enter to apply, Esc to cancel",
            m_tool.config().lightAngle,
            m_tool.config().ambientLevel * 100.0f);
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
    // Light points FROM the light source TOWARD the shape
    // =========================================================================
    double angleRad = config.lightAngle * (3.14159265358979323846 / 180.0);
    // 2D light direction (XY plane)
    double lightX = std::cos(angleRad);
    double lightY = -std::sin(angleRad);  // Negate for screen coords (Y down)
    // Z component: how much light comes from "above" (toward viewer)
    double lightZ = config.lightElevation;

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
        // STEP 2a: Calculate 3D normal using RADIAL SPHERICAL PROJECTION
        // Uses distance from center for direction and spherical math for depth
        // -----------------------------------------------------------------

        // Calculate radial direction from center to pixel
        double dx = pixel.x - shape.centerX;
        double dy = pixel.y - shape.centerY;
        double distFromCenter = std::sqrt(dx * dx + dy * dy);

        // Normalize to get radial direction (outward from center)
        double dirX = 0, dirY = -1;  // Default: up
        if (distFromCenter > 0.001) {
            dirX = dx / distFromCenter;
            dirY = dy / distFromCenter;
        }

        // Calculate normalized radius (0 at center, 1 at edge)
        double maxRadius = shape.maxDistance;
        if (maxRadius < 1.0) maxRadius = 1.0;
        double normDist = distFromCenter / maxRadius;
        if (normDist > 1.0) normDist = 1.0;

        // Apply roundness to control curvature
        double curvedDist = normDist;
        if (config.roundness > 0.001) {
            // Power function: higher roundness = more pronounced sphere effect
            curvedDist = std::pow(normDist, 1.0 / (config.roundness + 0.5));
        }

        // 3D normal for hemisphere projection:
        // - At center (r=0): normal points straight up (0, 0, 1)
        // - At edge (r=1): normal points outward (dirX, dirY, 0)
        // Using spherical formula: normalZ = sqrt(1 - r^2)
        double normalZ = std::sqrt(std::max(0.0, 1.0 - curvedDist * curvedDist));

        // XY normal follows radial direction, scaled by distance from center
        double normalX = dirX * curvedDist;
        double normalY = dirY * curvedDist;

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
        // STEP 2c: Check for reflected light on shadow-side edges
        // "Never let shadow extend all the way to the edge"
        // -----------------------------------------------------------------
        // Get distance from edge for reflected light calculation
        float distFromEdge = shape.getDistance(pixel);
        bool useReflectedLight = false;

        if (config.enableReflectedLight) {
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
        double adjustedIntensity = intensity;

        if (config.shadingMode == ShadingMode::ThreeShade ||
            config.shadingMode == ShadingMode::FiveShade) {
            // Project pixel position along the light direction axis
            // This gives us a "distance along light ray" value
            double lightDist = dx * lightX + dy * lightY;

            // The key insight: band boundaries should be perpendicular to light
            // We quantize the light distance to create clean stepped lines

            // Calculate the slope of the light direction for Bresenham stepping
            // For angles near 45°, we want 1:1 stepping
            // For angles near horizontal/vertical, we want longer runs
            double absLightX = std::abs(lightX);
            double absLightY = std::abs(lightY);

            // Determine the major axis and calculate step ratio
            double stepRatio;
            int majorCoord, minorCoord;
            if (absLightX > absLightY) {
                // More horizontal - major axis is X
                stepRatio = absLightY / (absLightX + 0.001);
                majorCoord = static_cast<int>(pixel.x);
                minorCoord = static_cast<int>(pixel.y);
            } else {
                // More vertical - major axis is Y
                stepRatio = absLightX / (absLightY + 0.001);
                majorCoord = static_cast<int>(pixel.y);
                minorCoord = static_cast<int>(pixel.x);
            }

            // Bresenham error accumulation for clean line stepping
            // This creates the classic pixel-perfect diagonal pattern
            double error = (majorCoord * stepRatio) - std::floor(majorCoord * stepRatio);

            // Adjust threshold based on accumulated error
            // This snaps the band boundary to the nearest clean pixel line
            double thresholdAdjust = 0.0;
            if (error < 0.33) {
                thresholdAdjust = -0.015;
            } else if (error > 0.67) {
                thresholdAdjust = 0.015;
            }

            // Also consider the perpendicular position for sub-pixel accuracy
            double perpX = -lightY;
            double perpY = lightX;
            double perpDist = dx * perpX + dy * perpY;
            int perpInt = static_cast<int>(std::round(perpDist));

            // Alternate adjustment based on perpendicular position
            // This creates consistent stepping across parallel boundary lines
            if (perpInt % 2 == 0) {
                thresholdAdjust *= 1.0;
            } else {
                thresholdAdjust *= -1.0;
            }

            adjustedIntensity = intensity + thresholdAdjust;
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
        // Invalidate the shape bounds
        gfx::Rect bounds = m_tool.lastShape().bounds;
        bounds.enlarge(2);
        editor->invalidateRect(editor->editorToScreen(bounds));
    }
    else {
        // Invalidate entire editor
        editor->invalidate();
    }
}

} // namespace app
