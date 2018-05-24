// Aseprite
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
#include "doc/brush.h"
#include "doc/image.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "gfx/region.h"

#include <climits>

namespace app {
namespace tools {

using namespace gfx;
using namespace doc;
using namespace filters;

ToolLoopManager::ToolLoopManager(ToolLoop* toolLoop)
  : m_toolLoop(toolLoop)
  , m_dirtyArea(toolLoop->getDirtyArea())
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

  // Convert the screen point to a sprite point
  Point spritePoint = pointer.point();
  m_toolLoop->setSpeed(Point(0, 0));
  m_oldPoint = spritePoint;
  snapToGrid(spritePoint);

  m_toolLoop->getController()->pressButton(m_stroke, spritePoint);

  std::string statusText;
  m_toolLoop->getController()->getStatusBarText(m_toolLoop, m_stroke, statusText);
  m_toolLoop->updateStatusBar(statusText.c_str());

  doLoopStep(false);
}

bool ToolLoopManager::releaseButton(const Pointer& pointer)
{
  m_lastPointer = pointer;

  if (isCanceled())
    return false;

  Point spritePoint = pointer.point();
  snapToGrid(spritePoint);

  bool res = m_toolLoop->getController()->releaseButton(m_stroke, spritePoint);

  if (!res && (m_toolLoop->getInk()->isSelection() ||
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

  // Convert the screen point to a sprite point
  Point spritePoint = pointer.point();
  // Calculate the speed (new sprite point - old sprite point)
  m_toolLoop->setSpeed(spritePoint - m_oldPoint);
  m_oldPoint = spritePoint;
  snapToGrid(spritePoint);

  m_toolLoop->getController()->movement(m_toolLoop, m_stroke, spritePoint);

  std::string statusText;
  m_toolLoop->getController()->getStatusBarText(m_toolLoop, m_stroke, statusText);
  m_toolLoop->updateStatusBar(statusText.c_str());

  doLoopStep(false);
}

void ToolLoopManager::doLoopStep(bool last_step)
{
  // Original set of points to interwine (original user stroke,
  // relative to sprite origin).
  Stroke main_stroke;
  if (!last_step)
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

  // Invalidate destionation image areas.
  if (m_toolLoop->getTracePolicy() == TracePolicy::Last) {
    // Copy source to destination (reset the previous trace). Useful
    // for tools like Line and Ellipse (we kept the last trace only).
    m_toolLoop->invalidateDstImage();
  }
  else if (m_toolLoop->getTracePolicy() == TracePolicy::AccumulateUpdateLast) {
    // Revalidate only this last dirty area (e.g. pixel-perfect
    // freehand algorithm needs this trace policy to redraw only the
    // last dirty area, which can vary in one pixel from the previous
    // tool loop cycle).
    m_toolLoop->invalidateDstImage(m_dirtyArea);
  }

  m_toolLoop->validateDstImage(m_dirtyArea);

  // Join or fill user points
  if (!m_toolLoop->getFilled() || (!last_step && !m_toolLoop->getPreviewFilled()))
    m_toolLoop->getIntertwine()->joinStroke(m_toolLoop, main_stroke);
  else
    m_toolLoop->getIntertwine()->fillStroke(m_toolLoop, main_stroke);

  if (m_toolLoop->getTracePolicy() == TracePolicy::Overlap) {
    // Copy destination to source (yes, destination to source). In
    // this way each new trace overlaps the previous one.
    m_toolLoop->copyValidDstToSrcImage(m_dirtyArea);
  }

  if (!m_dirtyArea.isEmpty())
    m_toolLoop->updateDirtyArea();
}

// Applies the grid settings to the specified sprite point.
void ToolLoopManager::snapToGrid(Point& point)
{
  if (!m_toolLoop->getController()->canSnapToGrid() ||
      !m_toolLoop->getSnapToGrid())
    return;

  point = snap_to_grid(m_toolLoop->getGridBounds(), point,
                       PreferSnapTo::ClosestGridVertex);
  point += m_toolLoop->getBrush()->center();
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

} // namespace tools
} // namespace app
