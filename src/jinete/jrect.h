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

#ifndef JINETE_JRECT_H_INCLUDED
#define JINETE_JRECT_H_INCLUDED

#include "jinete/jbase.h"

#define jrect_w(r) (((JRect)(r))->x2-((JRect)(r))->x1)
#define jrect_h(r) (((JRect)(r))->y2-((JRect)(r))->y1)

#define jrect_point_in(r,x,y)					\
  (((x) >= ((JRect)(r))->x1) && ((x) < ((JRect)(r))->x2) &&	\
   ((y) >= ((JRect)(r))->y1) && ((y) < ((JRect)(r))->y2))

struct jrect
{
  int x1, y1, x2, y2;
};

JRect jrect_new(int x1, int y1, int x2, int y2);
JRect jrect_new_copy(const JRect rect);
void jrect_free(JRect rect);

void jrect_copy(JRect dst, const JRect src);
void jrect_replace(JRect rect, int x1, int y1, int x2, int y2);

void jrect_union(JRect r1, const JRect r2);
bool jrect_intersect(JRect r1, const JRect r2);

void jrect_shrink(JRect rect, int border);
void jrect_stretch(JRect rect, int border);

void jrect_moveto(JRect rect, int x, int y);
void jrect_displace(JRect rect, int dx, int dy);

#endif
