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

#include <string.h>
#include "art_misc.h"
#include "art_uta.h"

/**
 * art_uta_new: Allocate a new uta.
 * @x0: Left coordinate of uta.
 * @y0: Top coordinate of uta.
 * @x1: Right coordinate of uta.
 * @y1: Bottom coordinate of uta.
 *
 * Allocates a new microtile array. The arguments are in units of
 * tiles, not pixels.
 *
 * Returns: the newly allocated #ArtUta.
 **/
ArtUta *
art_uta_new (int x0, int y0, int x1, int y1)
{
  ArtUta *uta;

  uta = art_new (ArtUta, 1);
  uta->x0 = x0;
  uta->y0 = y0;
  uta->width = x1 - x0;
  uta->height = y1 - y0;

  uta->utiles = art_new (ArtUtaBbox, uta->width * uta->height);

  memset (uta->utiles, 0, uta->width * uta->height * sizeof(ArtUtaBbox));
  return uta;
  }

/**
 * art_uta_new_coords: Allocate a new uta, based on pixel coordinates.
 * @x0: Left coordinate of uta.
 * @y0: Top coordinate of uta.
 * @x1: Right coordinate of uta.
 * @y1: Bottom coordinate of uta.
 *
 * Allocates a new microtile array. The arguments are in pixels
 *
 * Returns: the newly allocated #ArtUta.
 **/
ArtUta *
art_uta_new_coords (int x0, int y0, int x1, int y1)
{
  return art_uta_new (x0 >> ART_UTILE_SHIFT, y0 >> ART_UTILE_SHIFT,
		      1 + (x1 >> ART_UTILE_SHIFT),
		      1 + (y1 >> ART_UTILE_SHIFT));
}

/**
 * art_uta_free: Free a uta.
 * @uta: The uta to free.
 *
 * Frees the microtile array structure, including the actual microtile
 * data.
 **/
void
art_uta_free (ArtUta *uta)
{
  art_free (uta->utiles);
  art_free (uta);
}

/* User to Aardvark! */
