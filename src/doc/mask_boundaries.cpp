// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/mask_boundaries.h"

#include "doc/image_impl.h"

namespace doc {

void MaskBoundaries::reset()
{
  m_segs.clear();
  if (!m_path.isEmpty())
    m_path.rewind();
}

void MaskBoundaries::regen(const Image* bitmap)
{
  reset();

  int x, y, w = bitmap->width(), h = bitmap->height();

  const LockImageBits<BitmapTraits> bits(bitmap);
  auto it = bits.begin();       // Current pixel iterator
#if _DEBUG
  auto prevIt = bits.begin();   // Previous row iterator (same X pos)
#endif

  // Vertical segments being expanded from the previous row.
  std::vector<int> vertSegs(w+1, -1);

  // Horizontal segment being expanded from the previous column.
  int horzSeg;

#define new_hseg(open) {                                        \
    m_segs.push_back(Segment(open, gfx::Rect(x, y, 1, 0)));     \
    horzSeg = int(m_segs.size()-1);                             \
  }
#define new_vseg(open) {                                        \
    m_segs.push_back(Segment(open, gfx::Rect(x, y, 0, 1)));     \
    vertSegs[x] = int(m_segs.size()-1);                         \
  }
#define expand_hseg() { \
    ASSERT(hseg);       \
    ++hseg->m_bounds.w; \
  }
#define expand_vseg() { \
    ASSERT(vseg);       \
    ++vseg->m_bounds.h; \
  }
#define stop_expanding_hseg() {                 \
    horzSeg = -1;                               \
  }
#define stop_expanding_vseg() {                 \
    vertSegs[x] = -1;                           \
  }

  for (y=0; y<=h; ++y) {
    bool prevColor = false;         // Previous color (X-1) same Y row
    horzSeg = -1;

    for (x=0; x<=w; ++x) {
      bool color = (x < w && y < h && *it ? true: false);
#if _DEBUG
      bool prevRowColor = (x < w && y > 0 && *prevIt ? true: false);
#endif
      Segment* hseg = (horzSeg >= 0 ? &m_segs[horzSeg]: nullptr);
      Segment* vseg = (vertSegs[x] >= 0 ? &m_segs[vertSegs[x]]: nullptr);

      //
      // -   -
      //
      // -   1
      //
      if (color) {
        //
        // - | -
        //   o
        // -   1
        //
        if (vseg) {
          //
          // 0 | 1
          //   o
          // -   1
          //
          if (vseg->open()) {
            ASSERT(prevRowColor);

            //
            // 0 | 1
            // --x
            // 1   1
            //
            if (hseg) {
              ASSERT(hseg->open());
              ASSERT(prevColor);
              stop_expanding_hseg();
              stop_expanding_vseg();
            }
            //
            // 0 | 1
            //   |
            // 0 | 1
            //   o
            else {
              ASSERT(!prevColor);
              expand_vseg();
            }
          }
          //
          // 1 | 0
          //   x--o
          // -   1
          //
          else {
            ASSERT(!prevRowColor);

            //
            // 1 | 0
            // --x--o
            // 0 | 1
            //   o
            if (hseg) {
              ASSERT(!prevColor);
              ASSERT(!hseg->open());
              new_hseg(true);
              new_vseg(true);
            }
            //
            // 1 | 0
            //   x--o
            // 1   1
            //
            else {
              ASSERT(prevColor);
              new_hseg(true);
              stop_expanding_vseg();
            }
          }
        }
        //
        // -   -  (there is no vertical segment in this row, both colors are equal)
        //
        // -   1
        //
        else {
          //
          // -   -
          // --o
          // -   1
          //
          if (hseg) {
            //
            // 0   0
            // -----o
            // 1   1
            //
            if (hseg->open()) {
              ASSERT(prevColor);
              expand_hseg();
            }
            //
            // 1   1
            // --x
            // 0 | 1
            //   o
            else {
              ASSERT(!prevColor);
              stop_expanding_hseg();
              new_vseg(true);
            }
          }
          else {
            //
            // 1   1
            //
            // 1   1
            //
            if (prevColor) {
              // Do nothing, we are inside boundaries
            }
            //
            // 0   0
            //    --o
            // 0 | 1
            //   o
            else {
              // First two segments of a corner
              new_hseg(true);
              new_vseg(true);
            }
          }
        }
      }
      //
      // -   -
      //
      // -   0
      //
      else {
        //
        // - | -
        //   o
        // -   0
        //
        if (vseg) {
          //
          // 0 | 1
          //   o
          // -   0
          //
          if (vseg->open()) {
            ASSERT(prevRowColor);

            //
            // 0 | 1
            // --x--o
            // 1 | 0
            //   o
            if (hseg) {
              ASSERT(hseg->open());
              ASSERT(prevColor);
              new_hseg(false);
              new_vseg(false);
            }
            //
            // 0 | 1
            //   x--o
            // 0   0
            //
            else {
              ASSERT(!prevColor);
              new_hseg(false);
              stop_expanding_vseg();
            }
          }
          //
          // 1 | 0
          //   o
          // -   0
          //
          else {
            ASSERT(!prevRowColor);

            //
            // 1 | 0
            // --x
            // 0   0
            //
            if (hseg) {
              ASSERT(!prevColor);
              stop_expanding_hseg();
              stop_expanding_vseg();
            }
            //
            // 1 | 0
            //   |
            // 1 | 0
            //   o
            else {
              ASSERT(prevColor);
              expand_vseg();
            }
          }
        }
        //
        // -   -  (there is no vertical segment in this row, both colors are equal)
        //
        // -   0
        //
        else {
          //
          // -   -
          // --o
          // -   0
          //
          if (hseg) {
            //
            // 0   0
            // --x
            // 1 | 0
            //   o
            if (hseg->open()) {
              ASSERT(prevColor);
              stop_expanding_hseg();
              new_vseg(false);
            }
            //
            // 1   1
            // -----o
            // 0   0
            //
            else {
              ASSERT(!prevColor);
              expand_hseg();
            }
          }
          else {
            //
            // 1   1
            //    --o
            // 1 | 0
            //   o
            if (prevColor) {
              new_hseg(false);
              new_vseg(false);
            }
            //
            // 0   0
            //
            // 0   0
            //
            else {
              // Do nothing, we are inside boundaries
            }
          }
        }
      }

      prevColor = color;
      if (x < w) {
#if _DEBUG
        if (y > 0) ++prevIt;
#endif
        if (y < h) ++it;
      }
    }
  }

  ASSERT(it == bits.end());
  ASSERT(prevIt == bits.end());
}

void MaskBoundaries::offset(int x, int y)
{
  for (Segment& seg : m_segs)
    seg.offset(x, y);

  m_path.offset(x, y);
}

void MaskBoundaries::createPathIfNeeeded()
{
  if (!m_path.isEmpty())
    return;

  for (const auto& seg : m_segs) {
    gfx::Rect rc = seg.bounds();
    m_path.moveTo(rc.x, rc.y);

    if (seg.vertical())
      m_path.lineTo(rc.x, rc.y2());
    else
      m_path.lineTo(rc.x2(), rc.y);
  }
}

} // namespace doc
