/* Libart_LGPL - library of basic graphic primitives
 * Copyright (C) 1998 Raph Levien
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

/* Simple manipulations with affine transformations */

#include <math.h>
#include <stdio.h> /* for sprintf */
#include <string.h> /* for strcpy */
#include "art_misc.h"
#include "art_point.h"
#include "art_affine.h"


/* According to a strict interpretation of the libart structure, this
   routine should go into its own module, art_point_affine.  However,
   it's only two lines of code, and it can be argued that it is one of
   the natural basic functions of an affine transformation.
*/

/**
 * art_affine_point: Do an affine transformation of a point.
 * @dst: Where the result point is stored.
 * @src: The original point.
 @ @affine: The affine transformation.
 **/
void
art_affine_point (ArtPoint *dst, const ArtPoint *src,
		  const double affine[6])
{
  double x, y;

  x = src->x;
  y = src->y;
  dst->x = x * affine[0] + y * affine[2] + affine[4];
  dst->y = x * affine[1] + y * affine[3] + affine[5];
}

/**
 * art_affine_invert: Find the inverse of an affine transformation.
 * @dst: Where the resulting affine is stored.
 * @src: The original affine transformation.
 *
 * All non-degenerate affine transforms are invertible. If the original
 * affine is degenerate or nearly so, expect numerical instability and
 * very likely core dumps on Alpha and other fp-picky architectures.
 * Otherwise, @dst multiplied with @src, or @src multiplied with @dst
 * will be (to within roundoff error) the identity affine.
 **/
void
art_affine_invert (double dst[6], const double src[6])
{
  double r_det;

  r_det = 1.0 / (src[0] * src[3] - src[1] * src[2]);
  dst[0] = src[3] * r_det;
  dst[1] = -src[1] * r_det;
  dst[2] = -src[2] * r_det;
  dst[3] = src[0] * r_det;
  dst[4] = -src[4] * dst[0] - src[5] * dst[2];
  dst[5] = -src[4] * dst[1] - src[5] * dst[3];
}

/**
 * art_affine_flip: Flip an affine transformation horizontally and/or vertically.
 * @dst_affine: Where the resulting affine is stored.
 * @src_affine: The original affine transformation.
 * @horiz: Whether or not to flip horizontally.
 * @vert: Whether or not to flip horizontally.
 *
 * Flips the affine transform. FALSE for both @horiz and @vert implements
 * a simple copy operation. TRUE for both @horiz and @vert is a
 * 180 degree rotation. It is ok for @src_affine and @dst_affine to
 * be equal pointers.
 **/
void
art_affine_flip (double dst_affine[6], const double src_affine[6], int horz, int vert)
{
  dst_affine[0] = horz ? - src_affine[0] : src_affine[0];
  dst_affine[1] = horz ? - src_affine[1] : src_affine[1];
  dst_affine[2] = vert ? - src_affine[2] : src_affine[2];
  dst_affine[3] = vert ? - src_affine[3] : src_affine[3];
  dst_affine[4] = horz ? - src_affine[4] : src_affine[4];
  dst_affine[5] = vert ? - src_affine[5] : src_affine[5];
}

#define EPSILON 1e-6

/* It's ridiculous I have to write this myself. This is hardcoded to
   six digits of precision, which is good enough for PostScript.

   The return value is the number of characters (i.e. strlen (str)).
   It is no more than 12. */
static int
art_ftoa (char str[80], double x)
{
  char *p = str;
  int i, j;

  p = str;
  if (fabs (x) < EPSILON / 2)
    {
      strcpy (str, "0");
      return 1;
    }
  if (x < 0)
    {
      *p++ = '-';
      x = -x;
    }
  if ((int)floor ((x + EPSILON / 2) < 1))
    {
      *p++ = '0';
      *p++ = '.';
      i = sprintf (p, "%06d", (int)floor ((x + EPSILON / 2) * 1e6));
      while (i && p[i - 1] == '0')
	i--;
      if (i == 0)
	i--;
      p += i;
    }
  else if (x < 1e6)
    {
      i = sprintf (p, "%d", (int)floor (x + EPSILON / 2));
      p += i;
      if (i < 6)
	{
	  int ix;

	  *p++ = '.';
	  x -= floor (x + EPSILON / 2);
	  for (j = i; j < 6; j++)
	    x *= 10;
	  ix = floor (x + 0.5);

	  for (j = 0; j < i; j++)
	    ix *= 10;

	  /* A cheap hack, this routine can round wrong for fractions
	     near one. */
	  if (ix == 1000000)
	    ix = 999999;

	  sprintf (p, "%06d", ix);
	  i = 6 - i;
	  while (i && p[i - 1] == '0')
	    i--;
	  if (i == 0)
	    i--;
	  p += i;
	}
    }
  else
    p += sprintf (p, "%g", x);

  *p = '\0';
  return p - str;
}



#include <stdlib.h>
/**
 * art_affine_to_string: Convert affine transformation to concise PostScript string representation.
 * @str: Where to store the resulting string.
 * @src: The affine transform.
 *
 * Converts an affine transform into a bit of PostScript code that
 * implements the transform. Special cases of scaling, rotation, and
 * translation are detected, and the corresponding PostScript
 * operators used (this greatly aids understanding the output
 * generated). The identity transform is mapped to the null string.
 **/
void
art_affine_to_string (char str[128], const double src[6])
{
  char tmp[80];
  int i, ix;

#if 0
  for (i = 0; i < 1000; i++)
    {
      double d = rand () * .1 / RAND_MAX;
      art_ftoa (tmp, d);
      printf ("%g %f %s\n", d, d, tmp);
    }
#endif
  if (fabs (src[4]) < EPSILON && fabs (src[5]) < EPSILON)
    {
      /* could be scale or rotate */
      if (fabs (src[1]) < EPSILON && fabs (src[2]) < EPSILON)
	{
	  /* scale */
	  if (fabs (src[0] - 1) < EPSILON && fabs (src[3] - 1) < EPSILON)
	    {
	      /* identity transform */
	      str[0] = '\0';
	      return;
	    }
	  else
	    {
	      ix = 0;
	      ix += art_ftoa (str + ix, src[0]);
	      str[ix++] = ' ';
	      ix += art_ftoa (str + ix, src[3]);
	      strcpy (str + ix, " scale");
	      return;
	    }
	}
      else
	{
	  /* could be rotate */
	  if (fabs (src[0] - src[3]) < EPSILON &&
	      fabs (src[1] + src[2]) < EPSILON &&
	      fabs (src[0] * src[0] + src[1] * src[1] - 1) < 2 * EPSILON)
	    {
	      double theta;

	      theta = (180 / M_PI) * atan2 (src[1], src[0]);
	      art_ftoa (tmp, theta);
	      sprintf (str, "%s rotate", tmp);
	      return;
	    }
	}
    }
  else
    {
      /* could be translate */
      if (fabs (src[0] - 1) < EPSILON && fabs (src[1]) < EPSILON &&
	  fabs (src[2]) < EPSILON && fabs (src[3] - 1) < EPSILON)
	{
	  ix = 0;
	  ix += art_ftoa (str + ix, src[4]);
	  str[ix++] = ' ';
	  ix += art_ftoa (str + ix, src[5]);
	  strcpy (str + ix, " translate");
	  return;
	}
    }

  ix = 0;
  str[ix++] = '[';
  str[ix++] = ' ';
  for (i = 0; i < 6; i++)
    {
      ix += art_ftoa (str + ix, src[i]);
      str[ix++] = ' ';
    }
  strcpy (str + ix, "] concat");
}

/**
 * art_affine_multiply: Multiply two affine transformation matrices.
 * @dst: Where to store the result.
 * @src1: The first affine transform to multiply.
 * @src2: The second affine transform to multiply.
 *
 * Multiplies two affine transforms together, i.e. the resulting @dst
 * is equivalent to doing first @src1 then @src2. Note that the
 * PostScript concat operator multiplies on the left, i.e.  "M concat"
 * is equivalent to "CTM = multiply (M, CTM)";
 *
 * It is safe to call this function with @dst equal to @src1 or @src2.
 **/
void
art_affine_multiply (double dst[6], const double src1[6], const double src2[6])
{
  double d0, d1, d2, d3, d4, d5;

  d0 = src1[0] * src2[0] + src1[1] * src2[2];
  d1 = src1[0] * src2[1] + src1[1] * src2[3];
  d2 = src1[2] * src2[0] + src1[3] * src2[2];
  d3 = src1[2] * src2[1] + src1[3] * src2[3];
  d4 = src1[4] * src2[0] + src1[5] * src2[2] + src2[4];
  d5 = src1[4] * src2[1] + src1[5] * src2[3] + src2[5];
  dst[0] = d0;
  dst[1] = d1;
  dst[2] = d2;
  dst[3] = d3;
  dst[4] = d4;
  dst[5] = d5;
}

/**
 * art_affine_identity: Set up the identity matrix.
 * @dst: Where to store the resulting affine transform.
 *
 * Sets up an identity matrix.
 **/
void
art_affine_identity (double dst[6])
{
  dst[0] = 1;
  dst[1] = 0;
  dst[2] = 0;
  dst[3] = 1;
  dst[4] = 0;
  dst[5] = 0;
}


/**
 * art_affine_scale: Set up a scaling matrix.
 * @dst: Where to store the resulting affine transform.
 * @sx: X scale factor.
 * @sy: Y scale factor.
 *
 * Sets up a scaling matrix.
 **/
void
art_affine_scale (double dst[6], double sx, double sy)
{
  dst[0] = sx;
  dst[1] = 0;
  dst[2] = 0;
  dst[3] = sy;
  dst[4] = 0;
  dst[5] = 0;
}

/**
 * art_affine_rotate: Set up a rotation affine transform.
 * @dst: Where to store the resulting affine transform.
 * @theta: Rotation angle in degrees.
 *
 * Sets up a rotation matrix. In the standard libart coordinate
 * system, in which increasing y moves downward, this is a
 * counterclockwise rotation. In the standard PostScript coordinate
 * system, which is reversed in the y direction, it is a clockwise
 * rotation.
 **/
void
art_affine_rotate (double dst[6], double theta)
{
  double s, c;

  s = sin (theta * M_PI / 180.0);
  c = cos (theta * M_PI / 180.0);
  dst[0] = c;
  dst[1] = s;
  dst[2] = -s;
  dst[3] = c;
  dst[4] = 0;
  dst[5] = 0;
}

/**
 * art_affine_shear: Set up a shearing matrix.
 * @dst: Where to store the resulting affine transform.
 * @theta: Shear angle in degrees.
 *
 * Sets up a shearing matrix. In the standard libart coordinate system
 * and a small value for theta, || becomes \\. Horizontal lines remain
 * unchanged.
 **/
void
art_affine_shear (double dst[6], double theta)
{
  double t;

  t = tan (theta * M_PI / 180.0);
  dst[0] = 1;
  dst[1] = 0;
  dst[2] = t;
  dst[3] = 1;
  dst[4] = 0;
  dst[5] = 0;
}

/**
 * art_affine_translate: Set up a translation matrix.
 * @dst: Where to store the resulting affine transform.
 * @tx: X translation amount.
 * @tx: Y translation amount.
 *
 * Sets up a translation matrix.
 **/
void
art_affine_translate (double dst[6], double tx, double ty)
{
  dst[0] = 1;
  dst[1] = 0;
  dst[2] = 0;
  dst[3] = 1;
  dst[4] = tx;
  dst[5] = ty;
}

/**
 * art_affine_expansion: Find the affine's expansion factor.
 * @src: The affine transformation.
 *
 * Finds the expansion factor, i.e. the square root of the factor
 * by which the affine transform affects area. In an affine transform
 * composed of scaling, rotation, shearing, and translation, returns
 * the amount of scaling.
 *
 * Return value: the expansion factor.
 **/
double
art_affine_expansion (const double src[6])
{
  return sqrt (src[0] * src[3] - src[1] * src[2]);
}

/**
 * art_affine_rectilinear: Determine whether the affine transformation is rectilinear.
 * @src: The original affine transformation.
 *
 * Determines whether @src is rectilinear, i.e.  grid-aligned
 * rectangles are transformed to other grid-aligned rectangles.  The
 * implementation has epsilon-tolerance for roundoff errors.
 *
 * Return value: TRUE if @src is rectilinear.
 **/
int
art_affine_rectilinear (const double src[6])
{
  return ((fabs (src[1]) < EPSILON && fabs (src[2]) < EPSILON) ||
	  (fabs (src[0]) < EPSILON && fabs (src[3]) < EPSILON));
}

/**
 * art_affine_equal: Determine whether two affine transformations are equal.
 * @matrix1: An affine transformation.
 * @matrix2: Another affine transformation.
 *
 * Determines whether @matrix1 and @matrix2 are equal, with
 * epsilon-tolerance for roundoff errors.
 *
 * Return value: TRUE if @matrix1 and @matrix2 are equal.
 **/
int
art_affine_equal (double matrix1[6], double matrix2[6])
{
  return (fabs (matrix1[0] - matrix2[0]) < EPSILON &&
	  fabs (matrix1[1] - matrix2[1]) < EPSILON &&
	  fabs (matrix1[2] - matrix2[2]) < EPSILON &&
	  fabs (matrix1[3] - matrix2[3]) < EPSILON &&
	  fabs (matrix1[4] - matrix2[4]) < EPSILON &&
	  fabs (matrix1[5] - matrix2[5]) < EPSILON);
}
