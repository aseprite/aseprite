// The floodfill routine.
// By Shawn Hargreaves.
// Adapted to Aseprite by David Capello
// Added non-contiguous mode by David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "raster/algo.h"
#include "raster/image.h"
#include "raster/primitives.h"

#include <climits>
#include <cmath>
#include <vector>

namespace raster {

struct FLOODED_LINE {   // store segments which have been flooded
  short flags;          // status of the segment
  short lpos, rpos;     // left and right ends of segment
  short y;              // y coordinate of the segment
  int next;             // linked list if several per line
};

/* Note: a 'short' is not sufficient for 'next' above in some corner cases. */


static std::vector<FLOODED_LINE> flood_buf;
static int flood_count;          /* number of flooded segments */

#define FLOOD_IN_USE             1
#define FLOOD_TODO_ABOVE         2
#define FLOOD_TODO_BELOW         4

#define FLOOD_LINE(c)            (&flood_buf[c])

static inline bool color_equal_32(color_t c1, color_t c2, int tolerance)
{
  if (tolerance == 0)
    return (c1 == c2) || (rgba_geta(c1) == 0 && rgba_geta(c2) == 0);
  else {
    int r1 = rgba_getr(c1);
    int g1 = rgba_getg(c1);
    int b1 = rgba_getb(c1);
    int a1 = rgba_geta(c1);
    int r2 = rgba_getr(c2);
    int g2 = rgba_getg(c2);
    int b2 = rgba_getb(c2);
    int a2 = rgba_geta(c2);

    if (a1 == 0 && a2 == 0)
      return true;

    return ((ABS(r1-r2) <= tolerance) &&
            (ABS(g1-g2) <= tolerance) &&
            (ABS(b1-b2) <= tolerance) &&
            (ABS(a1-a2) <= tolerance));
  }
}

static inline bool color_equal_16(color_t c1, color_t c2, int tolerance)
{
  if (tolerance == 0)
    return (c1 == c2) || (graya_geta(c1) == 0 && graya_geta(c2) == 0);
  else {
    int k1 = graya_getv(c1);
    int a1 = graya_geta(c1);
    int k2 = graya_getv(c2);
    int a2 = graya_geta(c2);

    if (a1 == 0 && a2 == 0)
      return true;

    return ((ABS(k1-k2) <= tolerance) &&
            (ABS(a1-a2) <= tolerance));
  }
}

static inline bool color_equal_8(color_t c1, color_t c2, int tolerance)
{
  if (tolerance == 0)
    return (c1 == c2);
  else
    return ABS((int)c1 - (int)c2) <= tolerance;
}

template<typename ImageTraits>
static inline bool color_equal(color_t c1, color_t c2, int tolerance)
{
  static_assert(false && sizeof(ImageTraits), "Invalid color comparison");
}

template<>
inline bool color_equal<RgbTraits>(color_t c1, color_t c2, int tolerance)
{
  return color_equal_32(c1, c2, tolerance);
}

template<>
inline bool color_equal<GrayscaleTraits>(color_t c1, color_t c2, int tolerance)
{
  return color_equal_16(c1, c2, tolerance);
}

template<>
inline bool color_equal<IndexedTraits>(color_t c1, color_t c2, int tolerance)
{
  return color_equal_8(c1, c2, tolerance);
}



/* flooder:
 *  Fills a horizontal line around the specified position, and adds it
 *  to the list of drawn segments. Returns the first x coordinate after
 *  the part of the line which it has dealt with.
 */
static int flooder(Image *image, int x, int y,
                   color_t src_color, int tolerance, void *data, AlgoHLine proc)
{
  FLOODED_LINE *p;
  int left = 0, right = 0;
  int c;

  switch (image->pixelFormat()) {

    case IMAGE_RGB:
      {
        uint32_t* address = reinterpret_cast<uint32_t*>(image->getPixelAddress(0, y));

        // Check start pixel
        if (!color_equal_32((int)*(address+x), src_color, tolerance))
          return x+1;

        // Work left from starting point
        for (left=x-1; left>=0; left--) {
          if (!color_equal_32((int)*(address+left), src_color, tolerance))
            break;
        }

        // Work right from starting point
        for (right=x+1; right<image->width(); right++) {
          if (!color_equal_32((int)*(address+right), src_color, tolerance))
            break;
        }
      }
      break;

    case IMAGE_GRAYSCALE:
      {
        uint16_t* address = reinterpret_cast<uint16_t*>(image->getPixelAddress(0, y));

        // Check start pixel
        if (!color_equal_16((int)*(address+x), src_color, tolerance))
          return x+1;

        // Work left from starting point
        for (left=x-1; left>=0; left--) {
          if (!color_equal_16((int)*(address+left), src_color, tolerance))
            break;
        }

        // Work right from starting point
        for (right=x+1; right<image->width(); right++) {
          if (!color_equal_16((int)*(address+right), src_color, tolerance))
            break;
        }
      }
      break;

    case IMAGE_INDEXED:
      {
        uint8_t* address = image->getPixelAddress(0, y);

        // Check start pixel
        if (!color_equal_8((int)*(address+x), src_color, tolerance))
          return x+1;

        // Work left from starting point
        for (left=x-1; left>=0; left--) {
          if (!color_equal_8((int)*(address+left), src_color, tolerance))
            break;
        }

        // Work right from starting point
        for (right=x+1; right<image->width(); right++) {
          if (!color_equal_8((int)*(address+right), src_color, tolerance))
            break;
        }
      }
      break;

    default:
      // Check start pixel
      if (get_pixel(image, x, y) != src_color)
        return x+1;

      // Work left from starting point
      for (left=x-1; left>=0; left--) {
        if (get_pixel(image, left, y) != src_color)
          break;
      }

      // Work right from starting point
      for (right=x+1; right<image->width(); right++) {
        if (get_pixel(image, right, y) != src_color)
          break;
      }
      break;
  }

  left++;
  right--;

  /* draw the line */
  (*proc)(left, y, right, data);

  /* store it in the list of flooded segments */
  c = y;
  p = FLOOD_LINE(c);

  if (p->flags) {
    while (p->next) {
      c = p->next;
      p = FLOOD_LINE(c);
    }

    p->next = c = flood_count++;
    flood_buf.resize(flood_count);
    p = FLOOD_LINE(c);
  }

  p->flags = FLOOD_IN_USE;
  p->lpos = left;
  p->rpos = right;
  p->y = y;
  p->next = 0;

  if (y > 0)
    p->flags |= FLOOD_TODO_ABOVE;

  if (y+1 < image->height())
    p->flags |= FLOOD_TODO_BELOW;

  return right+2;
}



/* check_flood_line:
 *  Checks a line segment, using the scratch buffer is to store a list of
 *  segments which have already been drawn in order to minimise the required
 *  number of tests.
 */
static int check_flood_line(Image* image, int y, int left, int right,
                            int src_color, int tolerance, void *data, AlgoHLine proc)
{
  int c;
  FLOODED_LINE *p;
  int ret = false;

  while (left <= right) {
    c = y;

    for (;;) {
      p = FLOOD_LINE(c);

      if ((left >= p->lpos) && (left <= p->rpos)) {
        left = p->rpos+2;
        break;
      }

      c = p->next;

      if (!c) {
        left = flooder(image, left, y, src_color, tolerance, data, proc);
        ret = true;
        break;
      }
    }
  }

  return ret;
}

template<typename ImageTraits>
static void replace_color(Image* image, int src_color, int tolerance, void *data, AlgoHLine proc)
{
  typename ImageTraits::address_t address;
  int w = image->width();
  int h = image->height();

  for (int y=0; y<h; ++y) {
    address = reinterpret_cast<typename ImageTraits::address_t>(image->getPixelAddress(0, y));

    for (int x=0; x<w; ++x, ++address) {
      int right = -1;

      if (color_equal<ImageTraits>((int)(*address), src_color, tolerance)) {
        ++address;
        for (right=x+1; right<w; ++right, ++address) {
          if (!color_equal<ImageTraits>((int)(*address), src_color, tolerance))
            break;
        }
        (*proc)(x, y, right-1, data);
        x = right;
      }
    }
  }
}

/* floodfill:
 *  Fills an enclosed area (starting at point x, y) with the specified color.
 */
void algo_floodfill(Image* image, int x, int y,
  int tolerance, bool contiguous,
  void* data, AlgoHLine proc)
{
  // Make sure we have a valid starting point
  if ((x < 0) || (x >= image->width()) ||
      (y < 0) || (y >= image->height()))
    return;

  // What color to replace?
  color_t src_color = get_pixel(image, x, y);

  // Non-contiguous case, we replace colors in the whole image.
  if (!contiguous) {
    switch (image->pixelFormat()) {
      case IMAGE_RGB: replace_color<RgbTraits>(image, src_color, tolerance, data, proc); break;
      case IMAGE_GRAYSCALE: replace_color<GrayscaleTraits>(image, src_color, tolerance, data, proc); break;
      case IMAGE_INDEXED: replace_color<IndexedTraits>(image, src_color, tolerance, data, proc); break;
    }
    return;
  }

  /* set up the list of flooded segments */
  flood_buf.resize(image->height());
  flood_count = image->height();
  FLOODED_LINE* p = (FLOODED_LINE*)&flood_buf[0];
  for (int c=0; c<flood_count; c++) {
    p[c].flags = 0;
    p[c].lpos = SHRT_MAX;
    p[c].rpos = SHRT_MIN;
    p[c].y = y;
    p[c].next = 0;
  }

  /* start up the flood algorithm */
  flooder(image, x, y, src_color, tolerance, data, proc);

  /* continue as long as there are some segments still to test */
  bool done;
  do {
    done = true;

    /* for each line on the screen */
    for (int c=0; c<flood_count; c++) {

      p = FLOOD_LINE(c);

      /* check below the segment? */
      if (p->flags & FLOOD_TODO_BELOW) {
        p->flags &= ~FLOOD_TODO_BELOW;
        if (check_flood_line(image, p->y+1, p->lpos, p->rpos,
                             src_color, tolerance, data, proc)) {
          done = false;
          p = FLOOD_LINE(c);
        }
      }

      /* check above the segment? */
      if (p->flags & FLOOD_TODO_ABOVE) {
        p->flags &= ~FLOOD_TODO_ABOVE;
        if (check_flood_line(image, p->y-1, p->lpos, p->rpos,
                             src_color, tolerance, data, proc)) {
          done = false;
          /* special case shortcut for going backwards */
          if ((c < image->height()) && (c > 0))
            c -= 2;
        }
      }
    }
  } while (!done);
}

} // namespace raster
