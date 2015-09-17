// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
#include "app/tools/tool_loop.h"
#include "doc/image.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "gfx/region.h"

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

void ToolLoopManager::prepareLoop(const Pointer& pointer,
                                  ui::KeyModifiers modifiers)
{
  // Start with no points at all
  m_points.clear();

  // Prepare the ink
  m_toolLoop->getInk()->prepareInk(m_toolLoop);
  m_toolLoop->getIntertwine()->prepareIntertwine();
  m_toolLoop->getController()->prepareController(modifiers);
  m_toolLoop->getPointShape()->preparePointShape(m_toolLoop);
}

void ToolLoopManager::pressKey(ui::KeyScancode key)
{
  if (isCanceled())
    return;

  if (m_toolLoop->getController()->pressKey(key)) {
    if (m_lastPointer.getButton() != Pointer::None)
      movement(m_lastPointer);
  }
}

void ToolLoopManager::releaseKey(ui::KeyScancode key)
{
  if (isCanceled())
    return;

  if (key == ui::kKeyEsc) {
    m_toolLoop->cancel();
    return;
  }

  if (m_toolLoop->getController()->releaseKey(key)) {
    if (m_lastPointer.getButton() != Pointer::None)
      movement(m_lastPointer);
  }
}

void ToolLoopManager::pressButton(const Pointer& pointer)
{
  m_lastPointer = pointer;

  if (isCanceled())
    return;

  // If the user pressed the other mouse button...
  if ((m_toolLoop->getMouseButton() == ToolLoop::Left && pointer.getButton() == Pointer::Right) ||
      (m_toolLoop->getMouseButton() == ToolLoop::Right && pointer.getButton() == Pointer::Left)) {
    // Cancel the tool-loop (the destination image should be completelly discarded)
    m_toolLoop->cancel();
    return;
  }

  // Convert the screen point to a sprite point
  Point spritePoint = pointer.point();
  m_toolLoop->setSpeed(Point(0, 0));
  m_oldPoint = spritePoint;
  snapToGrid(spritePoint);

  m_toolLoop->getController()->pressButton(m_points, spritePoint);

  std::string statusText;
  m_toolLoop->getController()->getStatusBarText(m_points, statusText);
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

  bool res = m_toolLoop->getController()->releaseButton(m_points, spritePoint);

  if (!res && (m_toolLoop->getInk()->isSelection() || m_toolLoop->getFilled())) {
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

  m_toolLoop->getController()->movement(m_toolLoop, m_points, spritePoint);

  std::string statusText;
  m_toolLoop->getController()->getStatusBarText(m_points, statusText);
  m_toolLoop->updateStatusBar(statusText.c_str());

  doLoopStep(false);
}

void ToolLoopManager::doLoopStep(bool last_step)
{
  Points points_to_interwine;
  if (!last_step)
    m_toolLoop->getController()->getPointsToInterwine(m_points, points_to_interwine);
  else
    points_to_interwine = m_points;

  Point offset(m_toolLoop->getOffset());
  for (size_t i=0; i<points_to_interwine.size(); ++i)
    points_to_interwine[i] += offset;

  // Calculate the area to be updated in all document observers.
  calculateDirtyArea(points_to_interwine);

  // Validate source image area.
  if (m_toolLoop->getInk()->needsSpecialSourceArea()) {
    gfx::Region srcArea;
    m_toolLoop->getInk()->createSpecialSourceArea(m_dirtyArea, srcArea);
    m_toolLoop->validateSrcImage(srcArea);
  }
  else {
    m_toolLoop->validateSrcImage(m_dirtyArea);
  }

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

  // Get the modified area in the sprite with this intertwined set of points
  if (!m_toolLoop->getFilled() || (!last_step && !m_toolLoop->getPreviewFilled()))
    m_toolLoop->getIntertwine()->joinPoints(m_toolLoop, points_to_interwine);
  else
    m_toolLoop->getIntertwine()->fillPoints(m_toolLoop, points_to_interwine);

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

  point = snap_to_grid(m_toolLoop->getGridBounds(), point);
}

void ToolLoopManager::calculateDirtyArea(const Points& points)
{
  // Save the current dirty area if it's needed
  Region prevDirtyArea;
  if (m_toolLoop->getTracePolicy() == TracePolicy::Last)
    prevDirtyArea = m_dirtyArea;

  // Start with a fresh dirty area
  m_dirtyArea.clear();

  if (points.size() > 0) {
    Point minpt, maxpt;
    calculateMinMax(points, minpt, maxpt);

    // Expand the dirty-area with the pen width
    Rect r1, r2;
    m_toolLoop->getPointShape()->getModifiedArea(m_toolLoop, minpt.x, minpt.y, r1);
    m_toolLoop->getPointShape()->getModifiedArea(m_toolLoop, maxpt.x, maxpt.y, r2);

    m_dirtyArea.createUnion(m_dirtyArea, Region(r1.createUnion(r2)));
  }

  // Apply offset mode
  Point offset(m_toolLoop->getOffset());
  m_dirtyArea.offset(-offset);

  // Merge new dirty area with the previous one (for tools like line
  // or rectangle it's needed to redraw the previous position and
  // the new one)
  if (m_toolLoop->getTracePolicy() == TracePolicy::Last)
    m_dirtyArea.createUnion(m_dirtyArea, prevDirtyArea);

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

void ToolLoopManager::calculateMinMax(const Points& points, Point& minpt, Point& maxpt)
{
  ASSERT(points.size() > 0);

  minpt.x = points[0].x;
  minpt.y = points[0].y;
  maxpt.x = points[0].x;
  maxpt.y = points[0].y;

  for (size_t c=1; c<points.size(); ++c) {
    minpt.x = MIN(minpt.x, points[c].x);
    minpt.y = MIN(minpt.y, points[c].y);
    maxpt.x = MAX(maxpt.x, points[c].x);
    maxpt.y = MAX(maxpt.y, points[c].y);
  }
}

} // namespace tools
} // namespace app
