// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/autocrop.h"

#include "app/snap_to_grid.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "render/render.h"

#include <algorithm>
#include <memory>

namespace app {

using namespace doc;

namespace {

// We call a "solid border" when a specific color is repeated in every
// pixel of the image edge.  This will return isBorder1Solid=true if
// the top (when topBottomLookUp=true) or left edge (when
// topBottomLookUp=false) is a "solid border". In the case of
// isBorder2Solid is for the bottom (or right) edge.
bool analize_if_image_has_solid_borders(const Image* image,
                                        color_t& refColor,
                                        bool& isBorder1Solid,
                                        bool& isBorder2Solid,
                                        bool topBottomLookUp)
{
  isBorder1Solid = true;
  isBorder2Solid = true;

  int w = image->width();
  int h = image->height();
  const color_t probableRefColor1 = get_pixel(image, 0, 0);
  const color_t probableRefColor2 = get_pixel(image, w-1, h-1);
  color_t currentPixel;
  color_t transparentColor = image->maskColor();

  if (probableRefColor1 == transparentColor ||
      probableRefColor2 == transparentColor) {
    refColor = transparentColor;
    return false;
  }

  if (!topBottomLookUp)
    std::swap(w, h);

  for (int i=0; i<w; ++i) {
    if (topBottomLookUp)
      currentPixel = get_pixel(image, i, 0);
    else
      currentPixel = get_pixel(image, 0, i);
    if (currentPixel != probableRefColor1) {
      isBorder1Solid = false;
      if (currentPixel == transparentColor) {
        refColor = transparentColor;
        return false;
      }
    }
  }

  for (int i=0; i<w; ++i) {
    if (topBottomLookUp)
      currentPixel = get_pixel(image, i, h-1);
    else
      currentPixel = get_pixel(image, h-1, i);
    if (currentPixel != probableRefColor2) {
      isBorder2Solid = false;
      if (currentPixel == transparentColor) {
        refColor = transparentColor;
        return false;
      }
    }
  }
  return true;
}

} // anonymous namespace

bool get_shrink_rect(int *x1, int *y1, int *x2, int *y2,
                     Image *image, color_t refpixel)
{
#define SHRINK_SIDE(u_begin, u_op, u_final, u_add,              \
                    v_begin, v_op, v_final, v_add, U, V, var)   \
  do {                                                          \
    for (u = u_begin; u u_op u_final; u u_add) {                \
      for (v = v_begin; v v_op v_final; v v_add) {              \
        if (image->getPixel(U, V) != refpixel)                  \
          break;                                                \
      }                                                         \
      if (v == v_final)                                         \
        var;                                                    \
      else                                                      \
        break;                                                  \
    }                                                           \
  } while (0)

  int u, v;

  *x1 = 0;
  *y1 = 0;
  *x2 = image->width()-1;
  *y2 = image->height()-1;

  SHRINK_SIDE(0, <, image->width(), ++,
              0, <, image->height(), ++, u, v, (*x1)++);

  SHRINK_SIDE(0, <, image->height(), ++,
              0, <, image->width(), ++, v, u, (*y1)++);

  SHRINK_SIDE(image->width()-1, >, 0, --,
              0, <, image->height(), ++, u, v, (*x2)--);

  SHRINK_SIDE(image->height()-1, >, 0, --,
              0, <, image->width(), ++, v, u, (*y2)--);

  if ((*x1 > *x2) || (*y1 > *y2))
    return false;
  else
    return true;

#undef SHRINK_SIDE
}

bool get_shrink_rect2(int *x1, int *y1, int *x2, int *y2,
                      Image *image, Image *refimage)
{
#define SHRINK_SIDE(u_begin, u_op, u_final, u_add,              \
                    v_begin, v_op, v_final, v_add, U, V, var)   \
  do {                                                          \
    for (u = u_begin; u u_op u_final; u u_add) {                \
      for (v = v_begin; v v_op v_final; v v_add) {              \
        if (image->getPixel(U, V) != refimage->getPixel(U, V))  \
          break;                                                \
      }                                                         \
      if (v == v_final)                                         \
        var;                                                    \
      else                                                      \
        break;                                                  \
    }                                                           \
  } while (0)

  int u, v;

  *x1 = 0;
  *y1 = 0;
  *x2 = image->width()-1;
  *y2 = image->height()-1;

  SHRINK_SIDE(0, <, image->width(), ++,
              0, <, image->height(), ++, u, v, (*x1)++);

  SHRINK_SIDE(0, <, image->height(), ++,
              0, <, image->width(), ++, v, u, (*y1)++);

  SHRINK_SIDE(image->width()-1, >, 0, --,
              0, <, image->height(), ++, u, v, (*x2)--);

  SHRINK_SIDE(image->height()-1, >, 0, --,
              0, <, image->width(), ++, v, u, (*y2)--);

  if ((*x1 > *x2) || (*y1 > *y2))
    return false;
  else
    return true;

#undef SHRINK_SIDE
}

// A simple method to trim an image we have used in the past is
// selecting the top-left corner pixel as the "refColor" in
// shrink_bounds() algorithm (whatever that color is, transparent
// or non-transparent).
//
// Now we have changed it to other heuristic: we look if there are
// borders in the sprite with a solid color ("solid border"),
// i.e. a color repeating in every pixel of that specific side
// (left, top, right, or bottom).
//
// If we find a transparent pixel at the edges of the sprite, we
// automatically set "refColor" as the transparent color and it
// goes directly to shrink_bounds() function. Because this mean
// that we are in a transparent layer and the transparent color is
// the one that must be trimmed.
//
// The other case is when borders don't contain the transparent
// color, we search for a "solid border" (top border first), then
// it checks the opposite border (bottom border), then:
//
// 1) If the opposite border is equal to the first border,
//    the color of both borders will be the "refColor".
// 2) If the color of the opposite border is solid, BUT different to
//    the first border we will need the user intervention to select a
//    valid refColor (in this case the function returns false, which
//    means that we cannot automatically trim the image).
// 3) If opposite border contains differents colors, we choose the
//    first border color as "refColor".
// 4) It repeats the analysis with the left and right edges.
//
// If no border has solid color, trimSprite() does nothing.
bool get_best_refcolor_for_trimming(
  doc::Image* image,
  color_t& refColor)
{
  const color_t probableRefColor1 = get_pixel(image, 0, 0);
  const color_t probableRefColor2 = get_pixel(image, image->width()-1, image->height()-1);
  bool isBorder1Solid = true;
  bool isBorder2Solid = true;

  refColor = probableRefColor1;

  if (analize_if_image_has_solid_borders(
        image, refColor,
        isBorder1Solid,
        isBorder2Solid,
        true)) {                // Analize top vs. bottom borders
    // Here, we know that analize_if_image_has_solid_borders() did not
    // find transparent pixels on top and bottom borders.
    if (!isBorder1Solid &&
        isBorder2Solid) {
      refColor = probableRefColor2;
    }
    else if (isBorder1Solid &&
             isBorder2Solid &&
             probableRefColor1 != probableRefColor2) {
      // Both border are solid but with different colors, so the
      // decision should be asked to the user.
      return false;
    }

    if (analize_if_image_has_solid_borders(
          image, refColor,
          isBorder1Solid,
          isBorder2Solid,
          false)) {                // Analize left vs. right borders
      if (!isBorder1Solid &&
          isBorder2Solid)
        refColor = probableRefColor2;
      else if (isBorder1Solid &&
               isBorder2Solid &&
               probableRefColor1 != probableRefColor2)
        // Both border are solid but with different colors, so the
        // decision should be asked to the user.
        return false;
    }
  }
  return true;
}

gfx::Rect get_trimmed_bounds(
  const doc::Sprite* sprite,
  const bool byGrid)
{
  gfx::Rect bounds;

  std::unique_ptr<Image> image_wrap(Image::create(sprite->spec()));
  Image* image = image_wrap.get();

  render::Render render;

  for (frame_t frame(0); frame<sprite->totalFrames(); ++frame) {
    render.renderSprite(image, sprite, frame);

    gfx::Rect frameBounds;
    doc::color_t refColor;
    if (get_best_refcolor_for_trimming(image, refColor) &&
        doc::algorithm::shrink_bounds(image, frameBounds, refColor)) {
      bounds = bounds.createUnion(frameBounds);
    }

    // TODO merge this code with the code in DocExporter::captureSamples()
    if (byGrid) {
      const gfx::Rect& gridBounds = sprite->gridBounds();
      gfx::Point posTopLeft =
        snap_to_grid(gridBounds,
                     bounds.origin(),
                     PreferSnapTo::FloorGrid);
      gfx::Point posBottomRight =
        snap_to_grid(gridBounds,
                     bounds.point2(),
                     PreferSnapTo::CeilGrid);
      bounds = gfx::Rect(posTopLeft, posBottomRight);
    }
  }
  return bounds;
}

} // namespace app
