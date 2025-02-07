// Aseprite Document Library
// Copyright (c) 2019-2020 Igara Studio S.A.
// Copyright (c) 2001-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/algorithm/shift_image.h"

#include "base/pi.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/primitives.h"
#include "gfx/rect.h"

#include <vector>

namespace doc { namespace algorithm {

void shift_image(Image* image, int dx, int dy, double angle)
{
  gfx::Rect bounds(image->bounds());
  if (cos(angle) < -sqrt(2) / 2) {
    dx = -dx;
    dy = -dy;
  }
  else if (sin(angle) >= sqrt(2) / 2) {
    double aux;
    aux = dx;
    dx = -dy;
    dy = aux;
  }
  else if (sin(angle) < -sqrt(2) / 2) {
    double aux;
    aux = dx;
    dx = dy;
    dy = -aux;
  }
  // To simplify the algorithm we use a copy of the original image, we
  // could avoid this copy swapping rows and columns.
  ImageRef crop(crop_image(image, bounds.x, bounds.y, bounds.w, bounds.h, image->maskColor()));

  for (int y = 0; y < bounds.h; ++y) {
    for (int x = 0; x < bounds.w; ++x) {
      put_pixel(image,
                (bounds.w + dx + x) % bounds.w,
                (bounds.h + dy + y) % bounds.h,
                get_pixel(crop.get(), x, y));
    }
  }
}

ImageRef shift_image_with_mask(const Cel* cel,
                               const Mask* mask,
                               const int dx,
                               const int dy,
                               gfx::Rect& newCelBounds)
{
  ASSERT(!cel->bounds().isEmpty());
  ASSERT(!mask->bounds().isEmpty());

  // Making a image which bounds are equal to the UNION of cel->bounds() and mask->bounds()
  gfx::Rect compCelBounds = cel->bounds() | mask->bounds();
  ImageRef compImage(Image::create(cel->image()->pixelFormat(), compCelBounds.w, compCelBounds.h));

  // Making a Rect which represents the mask bounds of the original MASK relative to
  // the new COMPOUND IMAGE (compImage)
  gfx::Point maskCelGap(0, 0);
  if (cel->bounds().x < mask->bounds().x)
    maskCelGap.x = mask->bounds().x - cel->bounds().x;
  if (cel->bounds().y < mask->bounds().y)
    maskCelGap.y = mask->bounds().y - cel->bounds().y;
  gfx::Rect maskedBounds(maskCelGap.x, maskCelGap.y, mask->bounds().w, mask->bounds().h);

  // Making one combined image: Image with the Mask Bounds (unfilled spaces were filled with mask
  // color)
  compImage->copy(cel->image(),
                  gfx::Clip(cel->position().x - compCelBounds.x,
                            cel->position().y - compCelBounds.y,
                            0,
                            0,
                            cel->bounds().w,
                            cel->bounds().h));

  // Making a copy of only the image which will be shiftted
  ImageRef imageToShift(Image::create(compImage->pixelFormat(), maskedBounds.w, maskedBounds.h));
  imageToShift->copy(compImage.get(), gfx::Clip(0, 0, maskedBounds));

  // Shifting the masked area of the COMPOUND IMAGE (compImage).
  const int xInitial = maskedBounds.x;
  const int yInitial = maskedBounds.y;
  const int wMask = maskedBounds.w;
  const int hMask = maskedBounds.h;
  for (int y = 0; y < hMask; ++y) {
    for (int x = 0; x < wMask; ++x) {
      // Use floor modulo (Euclidean remainder).
      // Shifts are broken out and stored in separate variables
      // to make them easier to recognize and change in the event
      // that rem_eucl is implemented formally in the future.
      const int xShift = ((dx + x) % wMask + wMask) % wMask;
      const int yShift = ((dy + y) % hMask + hMask) % hMask;

      put_pixel(compImage.get(),
                xInitial + xShift,
                yInitial + yShift,
                get_pixel(imageToShift.get(), x, y));
    }
  }

  // Bounds and Image shrinking (we have to fit compound image (compImage) and bounds
  // (compCelBounds))
  gfx::Rect newBounds = compImage->bounds();
  if (algorithm::shrink_bounds(compImage.get(),
                               compImage->maskColor(),
                               // TODO adding the layer for tilemaps (so the program doesn't
                               //      crash), but anyway it the Shift algorithm is not
                               //      working yet for tilemaps
                               cel->layer(),
                               newBounds)) {
    compCelBounds.offset(newBounds.x, newBounds.y);
    compCelBounds.setSize(newBounds.size());
  }
  ImageRef finalImage(Image::create(compImage->pixelFormat(), compCelBounds.w, compCelBounds.h));
  finalImage->copy(compImage.get(),
                   gfx::Clip(0, 0, newBounds.x, newBounds.y, compCelBounds.w, compCelBounds.h));

  // Final cel content assign
  newCelBounds = compCelBounds;
  return finalImage;
}

}} // namespace doc::algorithm
