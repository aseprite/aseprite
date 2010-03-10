/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "jinete/jrect.h"

JRect jrect_new(int x1, int y1, int x2, int y2)
{
  JRect rect;

  rect = jnew(struct jrect, 1);
  if (!rect)
    return NULL;

  rect->x1 = x1;
  rect->y1 = y1;
  rect->x2 = x2;
  rect->y2 = y2;

  return rect;
}

JRect jrect_new_copy(const JRect rect)
{
  return jrect_new(rect->x1, rect->y1, rect->x2, rect->y2);
}

void jrect_free(JRect rect)
{
  jfree(rect);
}

void jrect_copy(JRect dst, const JRect src)
{
  dst->x1 = src->x1;
  dst->y1 = src->y1;
  dst->x2 = src->x2;
  dst->y2 = src->y2;
}

void jrect_replace(JRect rect, int x1, int y1, int x2, int y2)
{
  rect->x1 = x1;
  rect->y1 = y1;
  rect->x2 = x2;
  rect->y2 = y2;
}

void jrect_union(JRect r1, const JRect r2)
{
  r1->x1 = MIN(r1->x1, r2->x1);
  r1->y1 = MIN(r1->y1, r2->y1);
  r1->x2 = MAX(r1->x2, r2->x2);
  r1->y2 = MAX(r1->y2, r2->y2);
}

bool jrect_intersect(JRect r1, const JRect r2)
{
  if (!((r1->x2 <= r2->x1) || (r1->x1 >= r2->x2) ||
	(r1->y2 <= r2->y1) || (r1->y1 >= r2->y2))) {
    r1->x1 = MAX(r1->x1, r2->x1);
    r1->y1 = MAX(r1->y1, r2->y1);
    r1->x2 = MIN(r1->x2, r2->x2);
    r1->y2 = MIN(r1->y2, r2->y2);
    return true;
  }
  else
    return false;
}

void jrect_shrink(JRect rect, int border)
{
  rect->x1 += border;
  rect->y1 += border;
  rect->x2 -= border;
  rect->y2 -= border;
}

void jrect_stretch(JRect rect, int border)
{
  rect->x1 -= border;
  rect->y1 -= border;
  rect->x2 += border;
  rect->y2 += border;
}

void jrect_moveto(JRect rect, int x, int y)
{
  rect->x2 += x-rect->x1;
  rect->y2 += y-rect->y1;
  rect->x1 = x;
  rect->y1 = y;
}

void jrect_displace(JRect rect, int dx, int dy)
{
  rect->x1 += dx;
  rect->y1 += dy;
  rect->x2 += dx;
  rect->y2 += dy;
}
