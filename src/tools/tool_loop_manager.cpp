/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include "tools/tool_loop_manager.h"

#include "context.h"
#include "gfx/region.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "settings/document_settings.h"
#include "tools/controller.h"
#include "tools/ink.h"
#include "tools/intertwine.h"
#include "tools/point_shape.h"
#include "tools/tool_loop.h"
#include "util/render.h"

using namespace gfx;
using namespace tools;

ToolLoopManager::ToolLoopManager(ToolLoop* toolLoop)
  : m_toolLoop(toolLoop)
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
  m_points.clear();

  // Prepare the image where we will draw on
  image_copy(m_toolLoop->getDstImage(),
             m_toolLoop->getSrcImage(), 0, 0);

  // Prepare the ink
  m_toolLoop->getInk()->prepareInk(m_toolLoop);

  // Prepare preview image (the destination image will be our preview
  // in the tool-loop time, so we can see what we are drawing)
  RenderEngine::setPreviewImage(m_toolLoop->getLayer(),
                                m_toolLoop->getDstImage());
}

void ToolLoopManager::releaseLoop(const Pointer& pointer)
{
  // No more preview image
  RenderEngine::setPreviewImage(NULL, NULL);
}

void ToolLoopManager::pressButton(const Pointer& pointer)
{
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
  Point spritePoint = m_toolLoop->screenToSprite(Point(pointer.getX(), pointer.getY()));
  m_toolLoop->setSpeed(Point(0, 0));
  m_oldPoint = spritePoint;
  snapToGrid(true, spritePoint);

  m_toolLoop->getController()->pressButton(m_points, spritePoint);

  std::string statusText;
  m_toolLoop->getController()->getStatusBarText(m_points, statusText);
  m_toolLoop->updateStatusBar(statusText.c_str());

  doLoopStep(false);
}

bool ToolLoopManager::releaseButton(const Pointer& pointer)
{
  if (isCanceled())
    return false;

  Point spritePoint = m_toolLoop->screenToSprite(Point(pointer.getX(), pointer.getY()));
  snapToGrid(true, spritePoint);

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
  if (isCanceled())
    return;

  // Convert the screen point to a sprite point
  Point spritePoint = m_toolLoop->screenToSprite(Point(pointer.getX(), pointer.getY()));
  // Calculate the speed (new sprite point - old sprite point)
  m_toolLoop->setSpeed(spritePoint - m_oldPoint);
  m_oldPoint = spritePoint;
  snapToGrid(true, spritePoint);

  m_toolLoop->getController()->movement(m_toolLoop, m_points, spritePoint);

  std::string statusText;
  m_toolLoop->getController()->getStatusBarText(m_points, statusText);
  m_toolLoop->updateStatusBar(statusText.c_str());

  doLoopStep(false);
}

void ToolLoopManager::doLoopStep(bool last_step)
{
  static Region old_dirty_area;   // TODO Not thread safe

  Points points_to_interwine;
  if (!last_step)
    m_toolLoop->getController()->getPointsToInterwine(m_points, points_to_interwine);
  else
    points_to_interwine = m_points;

  Point offset(m_toolLoop->getOffset());
  for (size_t i=0; i<points_to_interwine.size(); ++i)
    points_to_interwine[i] += offset;

  switch (m_toolLoop->getTracePolicy()) {

    case TracePolicyAccumulate:
      // Do nothing. We accumulate traces in the destination image.
      break;

    case TracePolicyLast:
      // Copy source to destination (reset the previous trace). Useful
      // for tools like Line and Ellipse tools (we kept the last trace only).
      image_clear(m_toolLoop->getDstImage(), 0);
      image_copy(m_toolLoop->getDstImage(), m_toolLoop->getSrcImage(), 0, 0);
      break;

    case TracePolicyOverlap:
      // Copy destination to source (yes, destination to source). In
      // this way each new trace overlaps the previous one.
      image_copy(m_toolLoop->getSrcImage(), m_toolLoop->getDstImage(), 0, 0);
      break;
  }

  // Get the modified area in the sprite with this intertwined set of points
  if (!m_toolLoop->getFilled() || (!last_step && !m_toolLoop->getPreviewFilled()))
    m_toolLoop->getIntertwine()->joinPoints(m_toolLoop, points_to_interwine);
  else
    m_toolLoop->getIntertwine()->fillPoints(m_toolLoop, points_to_interwine);

  // Calculate the area to be updated in all document observers.
  Region& dirty_area = m_toolLoop->getDirtyArea();
  calculateDirtyArea(m_toolLoop, points_to_interwine, dirty_area);

  if (m_toolLoop->getTracePolicy() == TracePolicyLast) {
    dirty_area.createUnion(dirty_area, old_dirty_area);
    old_dirty_area = dirty_area;
  }

  if (!dirty_area.isEmpty())
    m_toolLoop->updateDirtyArea();
}

// Applies the grid settings to the specified sprite point, if
// "flexible" is true this function will try to snap the point
// to one of the four corners of each grid-tile, if "flexible"
// is false, only the origin of each grid-tile will be used
// to snap the point
void ToolLoopManager::snapToGrid(bool flexible, Point& point)
{
  if (!m_toolLoop->getController()->canSnapToGrid() ||
      !m_toolLoop->getDocumentSettings()->getSnapToGrid())
    return;

  m_toolLoop->getDocumentSettings()
    ->snapToGrid(point, (flexible ? SnapInRightBottom: NormalSnap));
}

void ToolLoopManager::calculateDirtyArea(ToolLoop* loop, const Points& points, Region& dirty_area)
{
  dirty_area.clear();

  if (points.size() > 0) {
    Point minpt, maxpt;
    calculateMinMax(points, minpt, maxpt);

    // Expand the dirty-area with the pen width
    Rect r1, r2;
    loop->getPointShape()->getModifiedArea(loop, minpt.x, minpt.y, r1);
    loop->getPointShape()->getModifiedArea(loop, maxpt.x, maxpt.y, r2);

    dirty_area.createUnion(dirty_area, Region(r1.createUnion(r2)));
  }

  // Apply offset mode
  Point offset(loop->getOffset());
  dirty_area.offset(-offset);

  // Apply tiled mode
  TiledMode tiledMode = loop->getDocumentSettings()->getTiledMode();
  if (tiledMode != TILED_NONE) {
    int w = loop->getSprite()->getWidth();
    int h = loop->getSprite()->getHeight();
    Region sprite_area(Rect(0, 0, w, h));
    Region outside;
    outside.createSubtraction(dirty_area, sprite_area);

    switch (tiledMode) {
      case TILED_X_AXIS:
        outside.createIntersection(outside, Region(Rect(-w*10000, 0, w*20000, h)));
        break;
      case TILED_Y_AXIS:
        outside.createIntersection(outside, Region(Rect(0, -h*10000, w, h*20000)));
        break;
    }

    Rect outsideBounds = outside.getBounds();
    if (outsideBounds.x < 0) outside.offset(w * (1+((-outsideBounds.x) / w)), 0);
    if (outsideBounds.y < 0) outside.offset(0, h * (1+((-outsideBounds.y) / h)));
    int x1 = outside.getBounds().x;

    while (true) {
      Region in_sprite;
      in_sprite.createIntersection(outside, sprite_area);
      outside.createSubtraction(outside, in_sprite);
      dirty_area.createUnion(dirty_area, in_sprite);

      outsideBounds = outside.getBounds();
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
