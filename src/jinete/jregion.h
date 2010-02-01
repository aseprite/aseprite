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

#ifndef JINETE_JREGION_H_INCLUDED
#define JINETE_JREGION_H_INCLUDED

#include "jinete/jrect.h"

#define JI_REGION_NIL(reg) ((reg)->data && !(reg)->data->numRects)
/* not a region */
#define JI_REGION_NAR(reg) ((reg)->data == &ji_broken_data)
#define JI_REGION_NUM_RECTS(reg) ((reg)->data ? (reg)->data->numRects : 1)
#define JI_REGION_SIZE(reg) ((reg)->data ? (reg)->data->size : 0)
#define JI_REGION_RECTS(reg) ((reg)->data ? (JRect)((reg)->data + 1) \
			      : &(reg)->extents)
#define JI_REGION_RECTPTR(reg) ((JRect)((reg)->data + 1))
#define JI_REGION_RECT(reg,i) (&JI_REGION_RECTPTR(reg)[i])
#define JI_REGION_TOP(reg) JI_REGION_RECT(reg, (reg)->data->numRects)
#define JI_REGION_END(reg) JI_REGION_RECT(reg, (reg)->data->numRects - 1)
#define JI_REGION_SZOF(n) (sizeof(struct jregion_data) + ((n) * sizeof(struct jrect)))

/* return values from jregion_rect_in */
#define JI_RGNOUT	0
#define JI_RGNIN	1
#define JI_RGNPART	2

struct jregion_data
{
  int size;
  int numRects;
  /* struct jrect rects[size]; */
};

struct jregion
{
  struct jrect extents;
  struct jregion_data *data;
};

extern struct jregion_data ji_broken_data;
extern struct jregion ji_broken_region;

JRegion jregion_new(JRect rect, int size);
void jregion_init(JRegion reg, JRect rect, int size);
void jregion_free(JRegion reg);
void jregion_uninit(JRegion reg);

bool jregion_copy(JRegion dst, JRegion src);
bool jregion_intersect(JRegion newrgn, JRegion reg1, JRegion reg2);
bool jregion_union(JRegion newrgn, JRegion reg1, JRegion reg2);
bool jregion_append(JRegion dstrgn, JRegion rgn);
bool jregion_validate(JRegion badreg, bool *overlap);

/* JRegion jrects_to_region(int nrects, JRect *prect, int ctype); */

bool jregion_subtract(JRegion regD, JRegion regM, JRegion regS);
bool jregion_inverse(JRegion newReg, JRegion reg1, JRect invRect);

int jregion_rect_in(JRegion region, JRect rect);
void jregion_translate(JRegion reg, int x, int y);

void jregion_reset(JRegion reg, JRect box);
bool jregion_break(JRegion reg);
bool jregion_point_in(JRegion reg, int x, int y, JRect box);

bool jregion_equal(JRegion reg1, JRegion reg2);
bool jregion_notempty(JRegion reg);
void jregion_empty(JRegion reg);
JRect jregion_extents(JRegion reg);

#endif
