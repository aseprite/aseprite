// The floodfill routine.
// By Shawn Hargreaves.
//
// Changes by David Capello:
// - Adapted to Aseprite
// - Added non-contiguous mode
// - Added mask parameter
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.
//
// TODO rewrite this algorithm from scratch

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/base.h"
#include "doc/algo.h"
#include "doc/image.h"
#include "doc/mask.h"
#include "doc/primitives.h"
#include "doc/primitives_fast.h"

#include <climits>
#include <cmath>
#include <vector>

namespace doc {
namespace algorithm {

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
  return false;
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
static int flooder(const Image* image,
                   const Mask* mask,
                   int x, int y,
                   const gfx::Rect& bounds,
                   color_t src_color, int tolerance, void *data, AlgoHLine proc)
{
#define MASKED(u, v)                                                    \
        (mask &&                                                        \
         (!mask->bounds().contains(u, v) ||                             \
          (mask->bitmap() &&                                            \
           !get_pixel_fast<BitmapTraits>(mask->bitmap(),                \
                                         (u)-mask->bounds().x,          \
                                         (v)-mask->bounds().y))))

  FLOODED_LINE *p;
  int left = 0, right = 0;
  int c;

  switch (image->pixelFormat()) {

    case IMAGE_RGB:
      {
        uint32_t* address = reinterpret_cast<uint32_t*>(image->getPixelAddress(0, y));

        // Check start pixel
        if (!color_equal_32((int)*(address+x), src_color, tolerance) || MASKED(x, y))
          return x+1;

        // Work left from starting point
        for (left=x-1; left>=bounds.x; left--) {
          if (!color_equal_32((int)*(address+left), src_color, tolerance) || MASKED(left, y))
            break;
        }

        // Work right from starting point
        for (right=x+1; right<bounds.x2(); right++) {
          if (!color_equal_32((int)*(address+right), src_color, tolerance) || MASKED(right, y))
            break;
        }
      }
      break;

    case IMAGE_GRAYSCALE:
      {
        uint16_t* address = reinterpret_cast<uint16_t*>(image->getPixelAddress(0, y));

        // Check start pixel
        if (!color_equal_16((int)*(address+x), src_color, tolerance) || MASKED(x, y))
          return x+1;

        // Work left from starting point
        for (left=x-1; left>=bounds.x; left--) {
          if (!color_equal_16((int)*(address+left), src_color, tolerance) || MASKED(left, y))
            break;
        }

        // Work right from starting point
        for (right=x+1; right<bounds.x2(); right++) {
          if (!color_equal_16((int)*(address+right), src_color, tolerance) || MASKED(right, y))
            break;
        }
      }
      break;

    case IMAGE_INDEXED:
      {
        uint8_t* address = image->getPixelAddress(0, y);

        // Check start pixel
        if (!color_equal_8((int)*(address+x), src_color, tolerance) || MASKED(x, y))
          return x+1;

        // Work left from starting point
        for (left=x-1; left>=bounds.x; left--) {
          if (!color_equal_8((int)*(address+left), src_color, tolerance) || MASKED(left, y))
            break;
        }

        // Work right from starting point
        for (right=x+1; right<bounds.x2(); right++) {
          if (!color_equal_8((int)*(address+right), src_color, tolerance) || MASKED(right, y))
            break;
        }
      }
      break;

    default:
      // Check start pixel
      if (get_pixel(image, x, y) != src_color || MASKED(x, y))
        return x+1;

      // Work left from starting point
      for (left=x-1; left>=bounds.x; left--) {
        if (get_pixel(image, left, y) != src_color || MASKED(left, y))
          break;
      }

      // Work right from starting point
      for (right=x+1; right<bounds.x2(); right++) {
        if (get_pixel(image, right, y) != src_color || MASKED(right, y))
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

  if (y > bounds.y)
    p->flags |= FLOOD_TODO_ABOVE;

  if (y+1 < bounds.y2())
    p->flags |= FLOOD_TODO_BELOW;

  return right+2;
}



/* check_flood_line:
 *  Checks a line segment, using the scratch buffer is to store a list of
 *  segments which have already been drawn in order to minimise the required
 *  number of tests.
 */
static int check_flood_line(const Image* image,
                            const Mask* mask,
                            int y, int left, int right,
                            const gfx::Rect& bounds,
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
        left = flooder(image, mask, left, y, bounds, src_color, tolerance, data, proc);
        ret = true;
        break;
      }
    }
  }

  return ret;
}

template<typename ImageTraits>
static void replace_color(const Image* image, const gfx::Rect& bounds, int src_color, int tolerance, void* data, AlgoHLine proc)
{
  typename ImageTraits::address_t address;

  for (int y=bounds.y; y<bounds.y2(); ++y) {
    address = reinterpret_cast<typename ImageTraits::address_t>(image->getPixelAddress(bounds.x, y));

    for (int x=bounds.x; x<bounds.x2(); ++x, ++address) {
      int right = -1;

      if (color_equal<ImageTraits>((int)(*address), src_color, tolerance)) {
        ++address;
        for (right=x+1; right<bounds.x2(); ++right, ++address) {
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
void floodfill(const Image* image,
               const Mask* mask,
               const int x, const int y,
               const gfx::Rect& bounds,
               const doc::color_t src_color,
               const int tolerance,
               const bool contiguous,
               const bool isEightConnected,
               void* data,
               AlgoHLine proc)
{
  // Make sure we have a valid starting point
  if ((x < 0) || (x >= image->width()) ||
      (y < 0) || (y >= image->height()))
    return;

  // Non-contiguous case, we replace colors in the whole image.
  if (!contiguous) {
    switch (image->pixelFormat()) {
      case IMAGE_RGB:
        replace_color<RgbTraits>(image, bounds, src_color, tolerance, data, proc);
        break;
      case IMAGE_GRAYSCALE:
        replace_color<GrayscaleTraits>(image, bounds, src_color, tolerance, data, proc);
        break;
      case IMAGE_INDEXED:
        replace_color<IndexedTraits>(image, bounds, src_color, tolerance, data, proc);
        break;
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

  // Start up the flood algorithm
  flooder(image, mask, x, y, bounds, src_color, tolerance, data, proc);

  // Continue as long as there are some segments still to test
  bool done;
  do {
    done = true;

    // For each line on the screen
    for (int c=0; c<flood_count; c++) {
      p = FLOOD_LINE(c);

      // Check below the segment?
      if (p->flags & FLOOD_TODO_BELOW) {
        p->flags &= ~FLOOD_TODO_BELOW;

        if (isEightConnected) {
          if (p->lpos+1 < bounds.x2() &&
              check_flood_line(image, mask, p->y+1, p->lpos+1, p->rpos, bounds,
                               src_color, tolerance, data, proc)) {
            done = false;
            p = FLOOD_LINE(c);
          }

          if (p->lpos-1 >= 0 &&
              check_flood_line(image, mask, p->y+1, p->lpos-1, p->rpos, bounds,
                               src_color, tolerance, data, proc)) {
            done = false;
            p = FLOOD_LINE(c);
          }

          if (p->rpos+1 < bounds.x2() &&
              check_flood_line(image, mask, p->y+1, p->lpos, p->rpos+1, bounds,
                               src_color, tolerance, data, proc)) {
            done = false;
            p = FLOOD_LINE(c);
          }

          if (p->rpos-1 >= 0 &&
              check_flood_line(image, mask, p->y+1, p->lpos, p->rpos-1, bounds,
                               src_color, tolerance, data, proc)) {
            done = false;
            p = FLOOD_LINE(c);
          }
        }

        if (check_flood_line(image, mask, p->y+1, p->lpos, p->rpos, bounds,
                             src_color, tolerance, data, proc)) {
          done = false;
          p = FLOOD_LINE(c);
        }
      }

      // Check above the segment?
      if (p->flags & FLOOD_TODO_ABOVE) {
        p->flags &= ~FLOOD_TODO_ABOVE;

        if (isEightConnected) {
          if (p->lpos+1 < bounds.x2() &&
              check_flood_line(image, mask, p->y-1, p->lpos+1, p->rpos, bounds,
                               src_color, tolerance, data, proc)) {
            done = false;
            p = FLOOD_LINE(c);
          }

          if (p->lpos-1 >= 0 &&
              check_flood_line(image, mask, p->y-1, p->lpos-1, p->rpos, bounds,
                               src_color, tolerance, data, proc)) {
            done = false;
            p = FLOOD_LINE(c);
          }

          if (p->rpos+1 < bounds.x2() &&
              check_flood_line(image, mask, p->y-1, p->lpos, p->rpos+1, bounds,
                               src_color, tolerance, data, proc)) {
            done = false;
            p = FLOOD_LINE(c);
          }

          if (p->rpos-1 >= 0 &&
              check_flood_line(image, mask, p->y-1, p->lpos, p->rpos-1, bounds,
                               src_color, tolerance, data, proc)) {
            done = false;
            p = FLOOD_LINE(c);
          }
        }

        if (check_flood_line(image, mask, p->y-1, p->lpos, p->rpos, bounds,
                             src_color, tolerance, data, proc)) {
          done = false;

          // Special case shortcut for going backwards
          if ((c > bounds.y) && (c < bounds.y2()))
            c -= 2;
        }
      }
    }
  } while (!done);
}

} // namespace algorithm
} // namespace doc
