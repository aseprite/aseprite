// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <string.h>

#include <allegro.h>

#include <stdlib.h>
#define xalloc			jmalloc
#define xfree			jfree
#define xrealloc		jrealloc

#include "gui/jbase.h"
#include "gui/jrect.h"
#include "gui/jregion.h"

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

