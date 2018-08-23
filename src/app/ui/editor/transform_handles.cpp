// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/transform_handles.h"

#include "app/pref/preferences.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "base/pi.h"
#include "os/surface.h"

namespace app {

using namespace app::skin;
using namespace ui;

static const int HANDLES = 8;

static struct HandlesInfo {
  // These indices are used to calculate the position of each handle.
  //
  //   handle.x = (corners[i1].x + corners[i2].x) / 2
  //   handle.y = (corners[i1].y + corners[i2].y) / 2
  //
  // Corners reference (i1, i2):
  //
  //   0,0  0,1  1,1
  //   0,3       1,2
  //   3,3  3,2  2,2
  //
  int i1, i2;
  // The angle bias of this specific handle.
  fixmath::fixed angle;
  // The exact handle type ([0] for scaling, [1] for rotating).
  HandleType handle[2];
} handles_info[HANDLES] = {
  { 1, 2,   0 << 16, { ScaleEHandle,  RotateEHandle } },
  { 1, 1,  32 << 16, { ScaleNEHandle, RotateNEHandle } },
  { 0, 1,  64 << 16, { ScaleNHandle,  RotateNHandle } },
  { 0, 0,  96 << 16, { ScaleNWHandle, RotateNWHandle } },
  { 0, 3, 128 << 16, { ScaleWHandle,  RotateWHandle } },
  { 3, 3, 160 << 16, { ScaleSWHandle, RotateSWHandle } },
  { 3, 2, 192 << 16, { ScaleSHandle,  RotateSHandle } },
  { 2, 2, 224 << 16, { ScaleSEHandle, RotateSEHandle } },
};

HandleType TransformHandles::getHandleAtPoint(Editor* editor, const gfx::Point& pt, const Transformation& transform)
{
  SkinTheme* theme = SkinTheme::instance();
  os::Surface* gfx = theme->parts.transformationHandle()->bitmap(0);
  fixmath::fixed angle = fixmath::ftofix(128.0 * transform.angle() / PI);

  Transformation::Corners corners;
  transform.transformBox(corners);

  std::vector<gfx::Point> screenPoints;
  getScreenPoints(editor, corners, screenPoints);

  int handle_rs[2] = { gfx->width()*2, gfx->width()*3 };
  for (int i=0; i<2; ++i) {
    int handle_r = handle_rs[i];
    for (size_t c=0; c<HANDLES; ++c) {
      if (inHandle(pt,
                   (screenPoints[handles_info[c].i1].x+screenPoints[handles_info[c].i2].x)/2,
                   (screenPoints[handles_info[c].i1].y+screenPoints[handles_info[c].i2].y)/2,
                   handle_r, handle_r,
                   angle + handles_info[c].angle)) {
        return handles_info[c].handle[i];
      }
    }
  }

  // Check if the cursor is in the pivot
  if (visiblePivot(angle) && getPivotHandleBounds(editor, transform, corners).contains(pt))
    return PivotHandle;

  return NoHandle;
}

void TransformHandles::drawHandles(Editor* editor, const Transformation& transform)
{
  ScreenGraphics g;
  fixmath::fixed angle = fixmath::ftofix(128.0 * transform.angle() / PI);

  Transformation::Corners corners;
  transform.transformBox(corners);

  std::vector<gfx::Point> screenPoints;
  getScreenPoints(editor, corners, screenPoints);

  // TODO DO NOT COMMIT
#if 0 // Uncomment this if you want to see the bounds in red (only for debugging purposes)
  // -----------------------------------------------
  {
    gfx::Point
      a(transform.bounds().origin()),
      b(transform.bounds().point2());
    a = editor->editorToScreen(a);
    b = editor->editorToScreen(b);
    g.drawRect(gfx::rgba(255, 0, 0), gfx::Rect(a, b));

    a = transform.pivot();
    a = editor->editorToScreen(a);
    g.drawRect(gfx::rgba(255, 0, 0), gfx::Rect(a.x-2, a.y-2, 5, 5));
  }
  // -----------------------------------------------
#endif

  // Draw corner handle
  for (size_t c=0; c<HANDLES; ++c) {
    drawHandle(&g,
      (screenPoints[handles_info[c].i1].x+screenPoints[handles_info[c].i2].x)/2,
      (screenPoints[handles_info[c].i1].y+screenPoints[handles_info[c].i2].y)/2,
      angle + handles_info[c].angle);
  }

  // Draw the pivot
  if (visiblePivot(angle)) {
    gfx::Rect pivotBounds = getPivotHandleBounds(editor, transform, corners);
    SkinTheme* theme = SkinTheme::instance();
    os::Surface* part = theme->parts.pivotHandle()->bitmap(0);

    g.drawRgbaSurface(part, pivotBounds.x, pivotBounds.y);
  }
}

void TransformHandles::invalidateHandles(Editor* editor, const Transformation& transform)
{
  SkinTheme* theme = SkinTheme::instance();
  fixmath::fixed angle = fixmath::ftofix(128.0 * transform.angle() / PI);

  Transformation::Corners corners;
  transform.transformBox(corners);

  std::vector<gfx::Point> screenPoints;
  getScreenPoints(editor, corners, screenPoints);

  // Invalidate each corner handle.
  for (size_t c=0; c<HANDLES; ++c) {
    os::Surface* part = theme->parts.transformationHandle()->bitmap(0);
    int u = (screenPoints[handles_info[c].i1].x+screenPoints[handles_info[c].i2].x)/2;
    int v = (screenPoints[handles_info[c].i1].y+screenPoints[handles_info[c].i2].y)/2;

    adjustHandle(u, v, part->width(), part->height(), angle + handles_info[c].angle);

    editor->invalidateRect(gfx::Rect(u, v, part->width(), part->height()));
  }

  // Invalidate area where the pivot is.
  if (visiblePivot(angle)) {
    gfx::Rect pivotBounds = getPivotHandleBounds(editor, transform, corners);
    os::Surface* part = theme->parts.pivotHandle()->bitmap(0);

    editor->invalidateRect(
      gfx::Rect(pivotBounds.x, pivotBounds.y,
                part->width(), part->height()));
  }
}

gfx::Rect TransformHandles::getPivotHandleBounds(Editor* editor,
                                                 const Transformation& transform,
                                                 const Transformation::Corners& corners)
{
  SkinTheme* theme = SkinTheme::instance();
  gfx::Size partSize = theme->parts.pivotHandle()->size();
  gfx::Point screenPivotPos = editor->editorToScreen(gfx::Point(transform.pivot()));

  screenPivotPos.x += editor->projection().applyX(1) / 2;
  screenPivotPos.y += editor->projection().applyY(1) / 2;

  return gfx::Rect(
    screenPivotPos.x-partSize.w/2,
    screenPivotPos.y-partSize.h/2,
    partSize.w,
    partSize.h);
}

bool TransformHandles::inHandle(const gfx::Point& pt, int x, int y, int gfx_w, int gfx_h, fixmath::fixed angle)
{
  adjustHandle(x, y, gfx_w, gfx_h, angle);

  return (pt.x >= x && pt.x < x+gfx_w &&
          pt.y >= y && pt.y < y+gfx_h);
}

void TransformHandles::drawHandle(Graphics* g, int x, int y, fixmath::fixed angle)
{
  SkinTheme* theme = SkinTheme::instance();
  os::Surface* part = theme->parts.transformationHandle()->bitmap(0);

  adjustHandle(x, y, part->width(), part->height(), angle);

  g->drawRgbaSurface(part, x, y);
}

void TransformHandles::adjustHandle(int& x, int& y, int handle_w, int handle_h, fixmath::fixed angle)
{
  angle = fixmath::fixadd(angle, fixmath::itofix(16));
  angle &= (255<<16);
  angle >>= 16;
  angle /= 32;

  // Adjust x,y position depending the angle of the handle
  switch (angle) {

    case 0:
      y = y-handle_h/2;
      break;

    case 1:
      y = y-handle_h;
      break;

    case 2:
      x = x-handle_w/2;
      y = y-handle_h;
      break;

    case 3:
      x = x-handle_w;
      y = y-handle_h;
      break;

    case 4:
      x = x-handle_w;
      y = y-handle_h/2;
      break;

    case 5:
      x = x-handle_w;
      break;

    case 6:
      x = x-handle_w/2;
      break;

    case 7:
      // x and y are correct
      break;
  }
}

bool TransformHandles::visiblePivot(fixmath::fixed angle) const
{
  return (Preferences::instance().selection.pivotVisibility() || angle != 0);
}

void TransformHandles::getScreenPoints(
  Editor* editor,
  const Transformation::Corners& corners,
  std::vector<gfx::Point>& screenPoints) const
{
  gfx::Point main = editor->mainTilePosition();

  screenPoints.resize(corners.size());
  for (size_t c=0; c<corners.size(); ++c)
    screenPoints[c] = editor->editorToScreen(
      gfx::Point((int)corners[c].x+main.x,
                 (int)corners[c].y+main.y));
}

} // namespace app
