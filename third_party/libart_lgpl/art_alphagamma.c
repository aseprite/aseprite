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

/* Some functions to build alphagamma tables */

#include <math.h>
#include "art_misc.h"

#include "art_alphagamma.h"

/**
 * art_alphagamma_new: Create a new #ArtAlphaGamma.
 * @gamma: Gamma value.
 *
 * Create a new #ArtAlphaGamma for a specific value of @gamma. When
 * correctly implemented (which is generally not the case in libart),
 * alpha compositing with an alphagamma parameter is equivalent to
 * applying the gamma transformation to source images, doing the alpha
 * compositing (in linear intensity space), then applying the inverse
 * gamma transformation, bringing it back to a gamma-adjusted
 * intensity space.
 *
 * Return value: The newly created #ArtAlphaGamma.
 **/
ArtAlphaGamma *
art_alphagamma_new (double gamma)
{
  int tablesize;
  ArtAlphaGamma *alphagamma;
  int i;
  int *table;
  art_u8 *invtable;
  double s, r_gamma;

  tablesize = ceil (gamma * 8);
  if (tablesize < 10)
    tablesize = 10;

  alphagamma = (ArtAlphaGamma *)art_alloc (sizeof(ArtAlphaGamma) +
					   ((1 << tablesize) - 1) *
					   sizeof(art_u8));
  alphagamma->gamma = gamma;
  alphagamma->invtable_size = tablesize;

  table = alphagamma->table;
  for (i = 0; i < 256; i++)
    table[i] = (int)floor (((1 << tablesize) - 1) *
			   pow (i * (1.0 / 255), gamma) + 0.5);

  invtable = alphagamma->invtable;
  s = 1.0 / ((1 << tablesize) - 1);
  r_gamma = 1.0 / gamma;
  for (i = 0; i < 1 << tablesize; i++)
    invtable[i] = (int)floor (255 * pow (i * s, r_gamma) + 0.5);

  return alphagamma;
}

/**
 * art_alphagamma_free: Free an #ArtAlphaGamma.
 * @alphagamma: An #ArtAlphaGamma.
 *
 * Frees the #ArtAlphaGamma.
 **/
void
art_alphagamma_free (ArtAlphaGamma *alphagamma)
{
  art_free (alphagamma);
}
