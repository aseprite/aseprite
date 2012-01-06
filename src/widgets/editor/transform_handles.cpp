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

#include "widgets/editor/transform_handles.h"

#include "skin/skin_theme.h"
#include "widgets/editor/editor.h"

#include <allegro.h>

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
  fixed angle;
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

TransformHandles::TransformHandles()
{
}

TransformHandles::~TransformHandles()
{
}

HandleType TransformHandles::getHandleAtPoint(Editor* editor, const gfx::Point& pt, const gfx::Transformation& transform)
{
  SkinTheme* theme = static_cast<SkinTheme*>(CurrentTheme::get());
  BITMAP* gfx = theme->get_part(PART_TRANSFORMATION_HANDLE);
  fixed angle = ftofix(128.0 * transform.angle() / PI);

  gfx::Transformation::Corners corners;
  transform.transformBox(corners);

  std::vector<int> x(corners.size());
  std::vector<int> y(corners.size());
  for (size_t c=0; c<corners.size(); ++c)
    editor->editorToScreen(corners[c].x, corners[c].y, &x[c], &y[c]);

  int handle_rs[2] = { gfx->w*2, gfx->w*3 };
  for (int i=0; i<2; ++i) {
    int handle_r = handle_rs[i];
    for (size_t c=0; c<HANDLES; ++c) {
      if (inHandle(pt,
                   (x[handles_info[c].i1]+x[handles_info[c].i2])/2,
                   (y[handles_info[c].i1]+y[handles_info[c].i2])/2,
                   handle_r, handle_r,
                   angle + handles_info[c].angle)) {
        return handles_info[c].handle[i];
      }
    }
  }

  // Check if the cursor is in the pivot
  if (angle != 0 && getPivotHandleBounds(editor, transform, corners).contains(pt))
    return PivotHandle;

  return NoHandle;
}

void TransformHandles::drawHandles(Editor* editor, const gfx::Transformation& transform)
{
  BITMAP* bmp = ji_screen;
  fixed angle = ftofix(128.0 * transform.angle() / PI);

  gfx::Transformation::Corners corners;
  transform.transformBox(corners);

  std::vector<int> x(corners.size());
  std::vector<int> y(corners.size());
  for (size_t c=0; c<corners.size(); ++c)
    editor->editorToScreen(corners[c].x, corners[c].y, &x[c], &y[c]);

#if 0 // Uncomment this if you want to see the bounds in red (only for debugging purposes)
  // -----------------------------------------------
  {
    int x1, y1, x2, y2;
    x1 = transform.bounds().x;
    y1 = transform.bounds().y;
    x2 = x1 + transform.bounds().w;
    y2 = y1 + transform.bounds().h;
    editor->editorToScreen(x1, y1, &x1, &y1);
    editor->editorToScreen(x2, y2, &x2, &y2);
    rect(bmp, x1, y1, x2, y2, makecol(255, 0, 0));

    x1 = transform.pivot().x;
    y1 = transform.pivot().y;
    editor->editorToScreen(x1, y1, &x1, &y1);
    circle(bmp, x1, y1, 4, makecol(255, 0, 0));
  }
  // -----------------------------------------------
#endif

  // Draw corner handle
  for (size_t c=0; c<HANDLES; ++c) {
    drawHandle(bmp,
               (x[handles_info[c].i1]+x[handles_info[c].i2])/2,
               (y[handles_info[c].i1]+y[handles_info[c].i2])/2,
               angle + handles_info[c].angle);
  }

  // Draw the pivot
  if (angle != 0) {
    gfx::Rect pivotBounds = getPivotHandleBounds(editor, transform, corners);
    SkinTheme* theme = static_cast<SkinTheme*>(CurrentTheme::get());
    BITMAP* gfx = theme->get_part(PART_PIVOT_HANDLE);

    draw_sprite_ex(bmp, gfx, pivotBounds.x, pivotBounds.y, DRAW_SPRITE_TRANS, DRAW_SPRITE_NO_FLIP);
  }
}

gfx::Rect TransformHandles::getPivotHandleBounds(Editor* editor,
                                                 const gfx::Transformation& transform,
                                                 const gfx::Transformation::Corners& corners)
{
  SkinTheme* theme = static_cast<SkinTheme*>(CurrentTheme::get());
  BITMAP* gfx = theme->get_part(PART_PIVOT_HANDLE);
  int pvx, pvy;

  editor->editorToScreen(transform.pivot().x, transform.pivot().y, &pvx, &pvy);

  pvx += (1 << editor->getZoom()) / 2;
  pvy += (1 << editor->getZoom()) / 2;

  return gfx::Rect(pvx-gfx->w/2, pvy-gfx->h/2, gfx->w, gfx->h);
}

bool TransformHandles::inHandle(const gfx::Point& pt, int x, int y, int gfx_w, int gfx_h, fixed angle)
{
  SkinTheme* theme = static_cast<SkinTheme*>(CurrentTheme::get());
  BITMAP* gfx = theme->get_part(PART_TRANSFORMATION_HANDLE);

  adjustHandle(x, y, gfx_w, gfx_h, angle);

  return (pt.x >= x && pt.x < x+gfx_w &&
          pt.y >= y && pt.y < y+gfx_h);
}

void TransformHandles::drawHandle(BITMAP* bmp, int x, int y, fixed angle)
{
  SkinTheme* theme = static_cast<SkinTheme*>(CurrentTheme::get());
  BITMAP* gfx = theme->get_part(PART_TRANSFORMATION_HANDLE);

  adjustHandle(x, y, gfx->w, gfx->h, angle);

  draw_sprite_ex(bmp, gfx, x, y, DRAW_SPRITE_TRANS, DRAW_SPRITE_NO_FLIP);
}

void TransformHandles::adjustHandle(int& x, int& y, int handle_w, int handle_h, fixed angle)
{
  angle = fixadd(angle, itofix(16));
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
