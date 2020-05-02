// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/tool_loop_manager.h"

#include "app/context.h"
#include "app/snap_to_grid.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/intertwine.h"
#include "app/tools/point_shape.h"
#include "app/tools/symmetry.h"
#include "app/tools/tool_loop.h"
#include "app/tools/velocity.h"
#include "base/clamp.h"
#include "doc/brush.h"
#include "doc/image.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "gfx/point_io.h"
#include "gfx/rect_io.h"
#include "gfx/region.h"

#include <climits>

#define TOOL_TRACE(...) // TRACEARGS

namespace app {
namespace tools {

using namespace gfx;
using namespace doc;
using namespace filters;

ToolLoopManager::ToolLoopManager(ToolLoop* toolLoop)
  : m_toolLoop(toolLoop)
  , m_brush0(*toolLoop->getBrush())
  , m_dynamics(toolLoop->getDynamics())
{
}

ToolLoopManager::~ToolLoopManager()
{
}

bool ToolLoopManager::isCanceled() const
{
 return m_toolLoop->isCanceled();
}

void ToolLoopManager::prepareLoop(const Pointer& pointer)
{
  // Start with no points at all
  m_stroke.reset();

  // Prepare the ink
  m_toolLoop->getInk()->prepareInk(m_toolLoop);
  m_toolLoop->getController()->prepareController(m_toolLoop);
  m_toolLoop->getIntertwine()->prepareIntertwine();
  m_toolLoop->getPointShape()->preparePointShape(m_toolLoop);
}

void ToolLoopManager::notifyToolLoopModifiersChange()
{
  if (isCanceled())
    return;

  if (m_lastPointer.button() != Pointer::None)
    movement(m_lastPointer);
}

void ToolLoopManager::pressButton(const Pointer& pointer)
{
  TOOL_TRACE("ToolLoopManager::pressButton", pointer.point());

  // A little patch to memorize initial Trace Policy in the
  // current function execution.
  // When the initial trace policy is "Last" and then
  // changes to different trace policy at the end of
  // this function, the user confirms a line draw while he
  // is holding the SHIFT key.
  bool tracePolicyWasLast = false;
  if (m_toolLoop->getTracePolicy() == TracePolicy::Last)
    tracePolicyWasLast = true;

  m_lastPointer = pointer;

  if (isCanceled())
    return;

  // If the user pressed the other mouse button...
  if ((m_toolLoop->getMouseButton() == ToolLoop::Left && pointer.button() == Pointer::Right) ||
      (m_toolLoop->getMouseButton() == ToolLoop::Right && pointer.button() == Pointer::Left)) {
    // Cancel the tool-loop (the destination image should be completely discarded)
    m_toolLoop->cancel();
    return;
  }

  Stroke::Pt spritePoint = getSpriteStrokePt(pointer);
  m_toolLoop->getController()->pressButton(m_toolLoop, m_stroke, spritePoint);

  std::string statusText;
  m_toolLoop->getController()->getStatusBarText(m_toolLoop, m_stroke, statusText);
  m_toolLoop->updateStatusBar(statusText.c_str());

  // We evaluate if the trace policy has changed compared with
  // the initial trace policy.
  if (!(m_toolLoop->getTracePolicy() == TracePolicy::Last) &&
        tracePolicyWasLast) {
    // Do nothing. We do not need execute an additional doLoopStep
    // (which it want to accumulate more points in m_pts in function
    // joinStroke() from intertwiners.h)
    // This avoid double print of a line while the user holds down
    // the SHIFT key.
  }
  else
    doLoopStep(false);
}

bool ToolLoopManager::releaseButton(const Pointer& pointer)
{
  TOOL_TRACE("ToolLoopManager::releaseButton", pointer.point());

  m_lastPointer = pointer;

  if (isCanceled())
    return false;

  Stroke::Pt spritePoint = getSpriteStrokePt(pointer);
  bool res = m_toolLoop->getController()->releaseButton(m_stroke, spritePoint);

  if (!res && (m_toolLoop->getTracePolicy() == TracePolicy::Last ||
               m_toolLoop->getInk()->isSelection() ||
               m_toolLoop->getInk()->isSlice() ||
               m_toolLoop->getFilled())) {
    m_toolLoop->getInk()->setFinalStep(m_toolLoop, true);
    doLoopStep(true);
    m_toolLoop->getInk()->setFinalStep(m_toolLoop, false);
  }

  return res;
}

void ToolLoopManager::movement(const Pointer& pointer)
{
  m_lastPointer = pointer;

  if (isCanceled())
    return;

  Stroke::Pt spritePoint = getSpriteStrokePt(pointer);
  m_toolLoop->getController()->movement(m_toolLoop, m_stroke, spritePoint);

  std::string statusText;
  m_toolLoop->getController()->getStatusBarText(m_toolLoop, m_stroke, statusText);
  m_toolLoop->updateStatusBar(statusText.c_str());

  doLoopStep(false);
}

void ToolLoopManager::doLoopStep(bool lastStep)
{
  // Original set of points to interwine (original user stroke,
  // relative to sprite origin).
  Stroke main_stroke;
  if (!lastStep)
    m_toolLoop->getController()->getStrokeToInterwine(m_stroke, main_stroke);
  else
    main_stroke = m_stroke;

  // Calculate the area to be updated in all document observers.
  Symmetry* symmetry = m_toolLoop->getSymmetry();
  Strokes strokes;
  if (symmetry)
    symmetry->generateStrokes(main_stroke, strokes, m_toolLoop);
  else
    strokes.push_back(main_stroke);

  calculateDirtyArea(strokes);

  // If we are not in the last step (when the mouse button is
  // released) we are only showing a preview of the tool, so we can
  // limit the dirty area to the visible viewport bounds. In this way
  // the area using in validateoDstImage() can be a lot smaller.
  if (m_toolLoop->getTracePolicy() == TracePolicy::Last &&
      !lastStep &&
      // We cannot limit the dirty area for LineFreehandController (or
      // in any case that the trace policy is handled by the
      // controller) just in case the user is using the Pencil tool
      // and used Shift+Click to draw a line and the origin point of
      // the line is not in the viewport area (e.g. for a very long
      // line, or with a lot of zoom in the end so the origin is not
      // viewable, etc.).
      !m_toolLoop->getController()->handleTracePolicy()) {
    m_toolLoop->limitDirtyAreaToViewport(m_dirtyArea);
  }

  // Validate source image area.
  if (m_toolLoop->getInk()->needsSpecialSourceArea()) {
    gfx::Region srcArea;
    m_toolLoop->getInk()->createSpecialSourceArea(m_dirtyArea, srcArea);
    m_toolLoop->validateSrcImage(srcArea);
  }
  else {
    m_toolLoop->validateSrcImage(m_dirtyArea);
  }

  m_toolLoop->getInk()->prepareForStrokes(m_toolLoop, strokes);

  // True when we have to fill
  const bool fillStrokes =
    (m_toolLoop->getFilled() &&
     (lastStep || m_toolLoop->getPreviewFilled()));

  // Invalidate the whole destination image area.
  if (m_toolLoop->getTracePolicy() == TracePolicy::Last ||
      fillStrokes) {
    // Copy source to destination (reset all the previous
    // traces). Useful for tools like Line and Ellipse (we keep the
    // last trace only) or to draw the final result in contour tool
    // (the final result is filled).
    m_toolLoop->invalidateDstImage();
  }
  else if (m_toolLoop->getTracePolicy() == TracePolicy::AccumulateUpdateLast) {
    // Revalidate only this last dirty area (e.g. pixel-perfect
    // freehand algorithm needs this trace policy to redraw only the
    // last dirty area, which can vary in one pixel from the previous
    // tool loop cycle).
    if (m_toolLoop->getBrush()->type() != kImageBrushType) {
      m_toolLoop->invalidateDstImage(m_dirtyArea);
    }
    // For custom brush we revalidate the whole destination area so
    // the whole trace is redrawn from scratch.
    else {
      m_toolLoop->invalidateDstImage();
      m_toolLoop->validateDstImage(gfx::Region(m_toolLoop->getDstImage()->bounds()));
    }
  }

  m_toolLoop->validateDstImage(m_dirtyArea);

  // Join or fill user points
  if (fillStrokes)
    m_toolLoop->getIntertwine()->fillStroke(m_toolLoop, main_stroke);
  else
    m_toolLoop->getIntertwine()->joinStroke(m_toolLoop, main_stroke);

  if (m_toolLoop->getTracePolicy() == TracePolicy::Overlap) {
    // Copy destination to source (yes, destination to source). In
    // this way each new trace overlaps the previous one.
    m_toolLoop->copyValidDstToSrcImage(m_dirtyArea);
  }

  if (!m_dirtyArea.isEmpty())
    m_toolLoop->updateDirtyArea(m_dirtyArea);

  TOOL_TRACE("ToolLoopManager::doLoopStep dirtyArea", m_dirtyArea.bounds());
}

// Applies the grid settings to the specified sprite point.
void ToolLoopManager::snapToGrid(Stroke::Pt& pt)
{
  if (!m_toolLoop->getController()->canSnapToGrid() ||
      !m_toolLoop->getSnapToGrid() ||
      m_toolLoop->isSelectingTiles())
    return;

  gfx::Point point(pt.x, pt.y);
  point = snap_to_grid(m_toolLoop->getGridBounds(), point,
                       PreferSnapTo::ClosestGridVertex);
  point += m_toolLoop->getBrush()->center();
  pt.x = point.x;
  pt.y = point.y;
}

// Strokes are relative to sprite origin.
void ToolLoopManager::calculateDirtyArea(const Strokes& strokes)
{
  // Save the current dirty area if it's needed
  Region prevDirtyArea;
  if (m_toolLoop->getTracePolicy() == TracePolicy::Last)
    prevDirtyArea = m_nextDirtyArea;

  // Start with a fresh dirty area
  m_dirtyArea.clear();

  for (auto& stroke : strokes) {
    gfx::Rect strokeBounds =
      m_toolLoop->getIntertwine()->getStrokeBounds(m_toolLoop, stroke);

    if (strokeBounds.isEmpty())
      continue;

    // Expand the dirty-area with the pen width
    Rect r1, r2;

    m_toolLoop->getPointShape()->getModifiedArea(
      m_toolLoop,
      strokeBounds.x,
      strokeBounds.y, r1);

    m_toolLoop->getPointShape()->getModifiedArea(
      m_toolLoop,
      strokeBounds.x+strokeBounds.w-1,
      strokeBounds.y+strokeBounds.h-1, r2);

    m_dirtyArea.createUnion(m_dirtyArea, Region(r1.createUnion(r2)));
  }

  // Merge new dirty area with the previous one (for tools like line
  // or rectangle it's needed to redraw the previous position and
  // the new one)
  if (m_toolLoop->getTracePolicy() == TracePolicy::Last) {
    m_nextDirtyArea = m_dirtyArea;
    m_dirtyArea.createUnion(m_dirtyArea, prevDirtyArea);
  }

  // Apply tiled mode
  TiledMode tiledMode = m_toolLoop->getTiledMode();
  if (tiledMode != TiledMode::NONE) {
    int w = m_toolLoop->sprite()->width();
    int h = m_toolLoop->sprite()->height();
    Region sprite_area(Rect(0, 0, w, h));
    Region outside;
    outside.createSubtraction(m_dirtyArea, sprite_area);

    switch (tiledMode) {
      case TiledMode::X_AXIS:
        outside.createIntersection(outside, Region(Rect(-w*10000, 0, w*20000, h)));
        break;
      case TiledMode::Y_AXIS:
        outside.createIntersection(outside, Region(Rect(0, -h*10000, w, h*20000)));
        break;
    }

    Rect outsideBounds = outside.bounds();
    if (outsideBounds.x < 0) outside.offset(w * (1+((-outsideBounds.x) / w)), 0);
    if (outsideBounds.y < 0) outside.offset(0, h * (1+((-outsideBounds.y) / h)));
    int x1 = outside.bounds().x;

    while (true) {
      Region in_sprite;
      in_sprite.createIntersection(outside, sprite_area);
      outside.createSubtraction(outside, in_sprite);
      m_dirtyArea.createUnion(m_dirtyArea, in_sprite);

      outsideBounds = outside.bounds();
      if (outsideBounds.isEmpty())
        break;
      else if (outsideBounds.x+outsideBounds.w > w)
        outside.offset(-w, 0);
      else if (outsideBounds.y+outsideBounds.h > h)
        outside.offset(x1-outsideBounds.x, -h);
      else
        break;
    }
  }
}

Stroke::Pt ToolLoopManager::getSpriteStrokePt(const Pointer& pointer)
{
  // Convert the screen point to a sprite point
  Stroke::Pt spritePoint = pointer.point();
  spritePoint.size = m_brush0.size();
  spritePoint.angle = m_brush0.angle();

  // Center the input to some grid point if needed
  snapToGrid(spritePoint);

  // Control dynamic parameters through sensors
  if (useDynamics()) {
    adjustPointWithDynamics(pointer, spritePoint);
  }

  // Inform the original velocity vector to the ToolLoop
  m_toolLoop->setSpeed(gfx::Point(pointer.velocity().x,
                                  pointer.velocity().y));

  return spritePoint;
}

bool ToolLoopManager::useDynamics() const
{
  return (m_dynamics.isDynamic() &&
          !m_toolLoop->getFilled() &&
          m_toolLoop->getController()->isFreehand());
}

void ToolLoopManager::adjustPointWithDynamics(const Pointer& pointer,
                                              Stroke::Pt& pt)
{
  int size = pt.size;
  int angle = pt.angle;

  // Pressure
  bool hasP = (pointer.type() == Pointer::Type::Pen ||
               pointer.type() == Pointer::Type::Eraser);
  float p = 1.0f;
  if (hasP) {
    p = pointer.pressure();
    if (p < m_dynamics.minPressureThreshold) {
      p = 0.0f;
    }
    else if (p > m_dynamics.maxPressureThreshold ||
             // To avoid div by zero
             m_dynamics.minPressureThreshold == m_dynamics.maxPressureThreshold) {
      p = 1.0f;
    }
    else {
      p =
        (p - m_dynamics.minPressureThreshold) /
        (m_dynamics.maxPressureThreshold - m_dynamics.minPressureThreshold);
    }
  }
  ASSERT(p >= 0.0f && p <= 1.0f);
  p = base::clamp(p, 0.0f, 1.0f);

  // Velocity
  float v = pointer.velocity().magnitude() / VelocitySensor::kScreenPixelsForFullVelocity;
  v = base::clamp(v, 0.0f, 1.0f);
  if (v < m_dynamics.minVelocityThreshold) {
    v = 0.0f;
  }
  else if (v > m_dynamics.maxVelocityThreshold ||
           // To avoid div by zero
           m_dynamics.minVelocityThreshold == m_dynamics.maxVelocityThreshold) {
    v = 1.0f;
  }
  else {
    v =
      (v - m_dynamics.minVelocityThreshold) /
      (m_dynamics.maxVelocityThreshold - m_dynamics.minVelocityThreshold);
  }
  ASSERT(v >= 0.0f && v <= 1.0f);
  v = base::clamp(v, 0.0f, 1.0f);

  switch (m_dynamics.size) {
    case DynamicSensor::Pressure:
      if (hasP) size = (1.0f-p)*m_dynamics.minSize + p*size;
      break;
    case DynamicSensor::Velocity:
      size = (1.0f-v)*m_dynamics.minSize + v*size;
      break;
  }

  switch (m_dynamics.angle) {
    case DynamicSensor::Pressure:
      if (hasP) angle = (1.0f-p)*m_dynamics.minAngle + p*angle;
      break;
    case DynamicSensor::Velocity:
      angle = (1.0f-v)*m_dynamics.minAngle + v*angle;
      break;
  }

  switch (m_dynamics.gradient) {
    case DynamicSensor::Pressure:
      pt.gradient = p;
      break;
    case DynamicSensor::Velocity:
      pt.gradient = v;
      break;
  }

  pt.size = base::clamp(size, int(Brush::kMinBrushSize), int(Brush::kMaxBrushSize));
  pt.angle = base::clamp(angle, -180, 180);
}

} // namespace tools
} // namespace app
