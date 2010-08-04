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

#include <string.h>

#include <allegro.h>

#include <stdlib.h>
#define xalloc			jmalloc
#define xfree			jfree
#define xrealloc		jrealloc

#include "jinete/jbase.h"
#include "jinete/jrect.h"
#include "jinete/jregion.h"

#define Bool			bool
#define BoxRec			struct jrect
#define RegDataRec		struct jregion_data
#define RegionRec		struct jregion
#define BoxPtr			JRect
#define RegDataPtr		RegDataRec *
#define RegionPtr		JRegion
#define NullBox			((JRect)NULL)

#define REGION_NIL(reg)		JI_REGION_NIL(reg)
#define REGION_NAR(reg)		JI_REGION_NAR(reg)
#define REGION_NUM_RECTS(reg)	JI_REGION_NUM_RECTS(reg)
#define REGION_SIZE(reg)	JI_REGION_SIZE(reg)
#define REGION_RECTS(reg)	JI_REGION_RECTS(reg)
#define REGION_BOXPTR(reg)	JI_REGION_RECTPTR(reg)
#define REGION_BOX(reg,i)	JI_REGION_RECT(reg,i)
#define REGION_TOP(reg)		JI_REGION_TOP(reg)
#define REGION_END(reg)		JI_REGION_END(reg)
#define REGION_SZOF(n)		JI_REGION_SZOF(n)

#define rgnOUT			JI_RGNOUT
#define rgnIN			JI_RGNIN
#define rgnPART			JI_RGNPART

struct ji_point { int x, y; };
#define DDXPointRec		struct ji_point
#define DDXPointPtr		DDXPointRec *

#include <limits.h>
#if !defined(MAXSHORT) || !defined(MINSHORT) || \
  !defined(MAXINT) || !defined(MININT)
/*
 * Some implementations #define these through <math.h>, so preclude
 * #include'ing it later.
 */

#include <math.h>
#endif
#undef MAXSHORT
#define MAXSHORT SHRT_MAX
#undef MINSHORT
#define MINSHORT SHRT_MIN
#undef MAXINT
#define MAXINT INT_MAX
#undef MININT
#define MININT INT_MIN

// min/max macros
#ifndef min
#define min			MIN
#endif
#ifndef max
#define max			MAX
#endif

#define miBrokenData		ji_broken_data
#define miBrokenRegion		ji_broken_region

#define miRegionCreate		jregion_new
#define miRegionInit		jregion_init
#define miRegionDestroy		jregion_free
#define miRegionUninit		jregion_uninit

#define miRegionCopy		jregion_copy
#define miIntersect		jregion_intersect
#define miUnion			jregion_union
#define miRegionAppend		jregion_append
#define miRegionValidate	jregion_validate

/* JRegion jrects_to_region(int nrects, JRect *prect, int ctype); */

#define miSubtract		jregion_subtract
#define miInverse		jregion_inverse

#define miRectIn		jregion_rect_in
#define miTranslateRegion	jregion_translate

#define miRegionReset		jregion_reset
#define miRegionBreak		jregion_break
#define miPointInRegion		jregion_point_in

#define miRegionEqual		jregion_equal
#define miRegionNotEmpty	jregion_notempty
#define miRegionEmpty		jregion_empty
#define miRegionExtents		jregion_extents

#undef assert
#define assert			ASSERT

#include "miregion.cpp"

