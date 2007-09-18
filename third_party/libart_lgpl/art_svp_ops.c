/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1998-2000 Raph Levien
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#define noVERBOSE

/* Vector path set operations, over sorted vpaths. */

#include "art_misc.h"

#include "art_svp.h"
#include "art_vpath.h"
#include "art_svp_vpath.h"
#include "art_svp_wind.h"
#include "art_svp_ops.h"
#include "art_vpath_svp.h"

/* Merge the segments of the two svp's. The resulting svp will share
   segments with args passed in, so be super-careful with the
   allocation.  */
/**
 * art_svp_merge: Merge the segments of two svp's.
 * @svp1: One svp to merge.
 * @svp2: The other svp to merge.
 *
 * Merges the segments of two SVP's into a new one. The resulting
 * #ArtSVP data structure will share the segments of the argument
 * svp's, so it is probably a good idea to free it shallowly,
 * especially if the arguments will be freed with art_svp_free().
 *
 * Return value: The merged #ArtSVP.
 **/
static ArtSVP *
art_svp_merge (const ArtSVP *svp1, const ArtSVP *svp2)
{
  ArtSVP *svp_new;
  int ix;
  int ix1, ix2;

  svp_new = (ArtSVP *)art_alloc (sizeof(ArtSVP) +
				 (svp1->n_segs + svp2->n_segs - 1) *
				 sizeof(ArtSVPSeg));
  ix1 = 0;
  ix2 = 0;
  for (ix = 0; ix < svp1->n_segs + svp2->n_segs; ix++)
    {
      if (ix1 < svp1->n_segs &&
	  (ix2 == svp2->n_segs ||
	   art_svp_seg_compare (&svp1->segs[ix1], &svp2->segs[ix2]) < 1))
	svp_new->segs[ix] = svp1->segs[ix1++];
      else
	svp_new->segs[ix] = svp2->segs[ix2++];
    }

  svp_new->n_segs = ix;
  return svp_new;
}

#ifdef VERBOSE

#define XOFF 50
#define YOFF 700

static void
print_ps_vpath (ArtVpath *vpath)
{
  int i;

  for (i = 0; vpath[i].code != ART_END; i++)
    {
      switch (vpath[i].code)
	{
	case ART_MOVETO:
	  printf ("%g %g moveto\n", XOFF + vpath[i].x, YOFF - vpath[i].y);
	  break;
	case ART_LINETO:
	  printf ("%g %g lineto\n", XOFF + vpath[i].x, YOFF - vpath[i].y);
	  break;
	default:
	  break;
	}
    }
  printf ("stroke showpage\n");
}

#define DELT 4

static void
print_ps_svp (ArtSVP *vpath)
{
  int i, j;

  printf ("%% begin\n");
  for (i = 0; i < vpath->n_segs; i++)
    {
      printf ("%g setgray\n", vpath->segs[i].dir ? 0.7 : 0);
      for (j = 0; j < vpath->segs[i].n_points; j++)
	{
	  printf ("%g %g %s\n",
		  XOFF + vpath->segs[i].points[j].x,
		  YOFF - vpath->segs[i].points[j].y,
		  j ? "lineto" : "moveto");
	}
      printf ("%g %g moveto %g %g lineto %g %g lineto %g %g lineto stroke\n",
	      XOFF + vpath->segs[i].points[0].x - DELT,
	      YOFF - DELT - vpath->segs[i].points[0].y,
	      XOFF + vpath->segs[i].points[0].x - DELT,
	      YOFF - vpath->segs[i].points[0].y,
	      XOFF + vpath->segs[i].points[0].x + DELT,
	      YOFF - vpath->segs[i].points[0].y,
	      XOFF + vpath->segs[i].points[0].x + DELT,
	      YOFF - DELT - vpath->segs[i].points[0].y);
      printf ("%g %g moveto %g %g lineto %g %g lineto %g %g lineto stroke\n",
	      XOFF + vpath->segs[i].points[j - 1].x - DELT,
	      YOFF + DELT - vpath->segs[i].points[j - 1].y,
	      XOFF + vpath->segs[i].points[j - 1].x - DELT,
	      YOFF - vpath->segs[i].points[j - 1].y,
	      XOFF + vpath->segs[i].points[j - 1].x + DELT,
	      YOFF - vpath->segs[i].points[j - 1].y,
	      XOFF + vpath->segs[i].points[j - 1].x + DELT,
	      YOFF + DELT - vpath->segs[i].points[j - 1].y);
      printf ("stroke\n");
    }

  printf ("showpage\n");
}
#endif

static ArtSVP *
art_svp_merge_perturbed (const ArtSVP *svp1, const ArtSVP *svp2)
{
  ArtVpath *vpath1, *vpath2;
  ArtVpath *vpath1_p, *vpath2_p;
  ArtSVP *svp1_p, *svp2_p;
  ArtSVP *svp_new;

  vpath1 = art_vpath_from_svp (svp1);
  vpath1_p = art_vpath_perturb (vpath1);
  art_free (vpath1);
  svp1_p = art_svp_from_vpath (vpath1_p);
  art_free (vpath1_p);

  vpath2 = art_vpath_from_svp (svp2);
  vpath2_p = art_vpath_perturb (vpath2);
  art_free (vpath2);
  svp2_p = art_svp_from_vpath (vpath2_p);
  art_free (vpath2_p);

  svp_new = art_svp_merge (svp1_p, svp2_p);
#ifdef VERBOSE
  print_ps_svp (svp1_p);
  print_ps_svp (svp2_p);
  print_ps_svp (svp_new);
#endif
  art_free (svp1_p);
  art_free (svp2_p);

  return svp_new;
}

/* Compute the union of two vector paths.

   Status of this routine:

   Basic correctness: Seems to work.

   Numerical stability: We cheat (adding random perturbation). Thus,
   it seems very likely that no numerical stability problems will be
   seen in practice.

   Speed: Would be better if we didn't go to unsorted vector path
   and back to add the perturbation.

   Precision: The perturbation fuzzes the coordinates slightly. In
   cases of butting segments, razor thin long holes may appear.

*/
/**
 * art_svp_union: Compute the union of two sorted vector paths.
 * @svp1: One sorted vector path.
 * @svp2: The other sorted vector path.
 *
 * Computes the union of the two argument svp's. Given two svp's with
 * winding numbers of 0 and 1 everywhere, the resulting winding number
 * will be 1 where either (or both) of the argument svp's has a
 * winding number 1, 0 otherwise. The result is newly allocated.
 *
 * Currently, this routine has accuracy problems pending the
 * implementation of the new intersector.
 *
 * Return value: The union of @svp1 and @svp2.
 **/
ArtSVP *
art_svp_union (const ArtSVP *svp1, const ArtSVP *svp2)
{
  ArtSVP *svp3, *svp4, *svp_new;

  svp3 = art_svp_merge_perturbed (svp1, svp2);
  svp4 = art_svp_uncross (svp3);
  art_svp_free (svp3);

  svp_new = art_svp_rewind_uncrossed (svp4, ART_WIND_RULE_POSITIVE);
#ifdef VERBOSE
  print_ps_svp (svp4);
  print_ps_svp (svp_new);
#endif
  art_svp_free (svp4);
  return svp_new;
}

/* Compute the intersection of two vector paths.

   Status of this routine:

   Basic correctness: Seems to work.

   Numerical stability: We cheat (adding random perturbation). Thus,
   it seems very likely that no numerical stability problems will be
   seen in practice.

   Speed: Would be better if we didn't go to unsorted vector path
   and back to add the perturbation.

   Precision: The perturbation fuzzes the coordinates slightly. In
   cases of butting segments, razor thin long isolated segments may
   appear.

*/

/**
 * art_svp_intersect: Compute the intersection of two sorted vector paths.
 * @svp1: One sorted vector path.
 * @svp2: The other sorted vector path.
 *
 * Computes the intersection of the two argument svp's. Given two
 * svp's with winding numbers of 0 and 1 everywhere, the resulting
 * winding number will be 1 where both of the argument svp's has a
 * winding number 1, 0 otherwise. The result is newly allocated.
 *
 * Currently, this routine has accuracy problems pending the
 * implementation of the new intersector.
 *
 * Return value: The intersection of @svp1 and @svp2.
 **/
ArtSVP *
art_svp_intersect (const ArtSVP *svp1, const ArtSVP *svp2)
{
  ArtSVP *svp3, *svp4, *svp_new;

  svp3 = art_svp_merge_perturbed (svp1, svp2);
  svp4 = art_svp_uncross (svp3);
  art_svp_free (svp3);

  svp_new = art_svp_rewind_uncrossed (svp4, ART_WIND_RULE_INTERSECT);
  art_svp_free (svp4);
  return svp_new;
}

/* Compute the symmetric difference of two vector paths.

   Status of this routine:

   Basic correctness: Seems to work.

   Numerical stability: We cheat (adding random perturbation). Thus,
   it seems very likely that no numerical stability problems will be
   seen in practice.

   Speed: We could do a lot better by scanning through the svp
   representations and culling out any segments that are exactly
   identical. It would also be better if we didn't go to unsorted
   vector path and back to add the perturbation.

   Precision: Awful. In the case of inputs which are similar (the
   common case for canvas display), the entire outline is "hairy." In
   addition, the perturbation fuzzes the coordinates slightly. It can
   be used as a conservative approximation.

*/

/**
 * art_svp_diff: Compute the symmetric difference of two sorted vector paths.
 * @svp1: One sorted vector path.
 * @svp2: The other sorted vector path.
 *
 * Computes the symmetric of the two argument svp's. Given two svp's
 * with winding numbers of 0 and 1 everywhere, the resulting winding
 * number will be 1 where either, but not both, of the argument svp's
 * has a winding number 1, 0 otherwise. The result is newly allocated.
 *
 * Currently, this routine has accuracy problems pending the
 * implementation of the new intersector.
 *
 * Return value: The symmetric difference of @svp1 and @svp2.
 **/
ArtSVP *
art_svp_diff (const ArtSVP *svp1, const ArtSVP *svp2)
{
  ArtSVP *svp3, *svp4, *svp_new;

  svp3 = art_svp_merge_perturbed (svp1, svp2);
  svp4 = art_svp_uncross (svp3);
  art_svp_free (svp3);

  svp_new = art_svp_rewind_uncrossed (svp4, ART_WIND_RULE_ODDEVEN);
  art_svp_free (svp4);
  return svp_new;
}

/* todo: implement minus */
