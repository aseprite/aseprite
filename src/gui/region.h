// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_REGION_H_INCLUDED
#define GUI_REGION_H_INCLUDED

#include "gui/rect.h"

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
