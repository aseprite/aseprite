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

#include "art_misc.h"
#include "art_uta.h"
#include "art_rect.h"
#include "art_uta_rect.h"

/**
 * art_uta_from_irect: Generate uta covering a rectangle.
 * @bbox: The source rectangle.
 *
 * Generates a uta exactly covering @bbox. Please do not call this
 * function with a @bbox with zero height or width.
 *
 * Return value: the new uta.
 **/
ArtUta *
art_uta_from_irect (ArtIRect *bbox)
{
  ArtUta *uta;
  ArtUtaBbox *utiles;
  ArtUtaBbox bb;
  int width, height;
  int x, y;
  int xf0, yf0, xf1, yf1;
  int ix;

  uta = art_new (ArtUta, 1);
  uta->x0 = bbox->x0 >> ART_UTILE_SHIFT;
  uta->y0 = bbox->y0 >> ART_UTILE_SHIFT;
  width = ((bbox->x1 + ART_UTILE_SIZE - 1) >> ART_UTILE_SHIFT) - uta->x0;
  height = ((bbox->y1 + ART_UTILE_SIZE - 1) >> ART_UTILE_SHIFT) - uta->y0;
  utiles = art_new (ArtUtaBbox, width * height);

  uta->width = width;
  uta->height = height;
  uta->utiles = utiles;

  xf0 = bbox->x0 & (ART_UTILE_SIZE - 1);
  yf0 = bbox->y0 & (ART_UTILE_SIZE - 1);
  xf1 = ((bbox->x1 - 1) & (ART_UTILE_SIZE - 1)) + 1;
  yf1 = ((bbox->y1 - 1) & (ART_UTILE_SIZE - 1)) + 1;
  if (height == 1)
    {
      if (width == 1)
	utiles[0] = ART_UTA_BBOX_CONS (xf0, yf0, xf1, yf1);
      else
	{
	  utiles[0] = ART_UTA_BBOX_CONS (xf0, yf0, ART_UTILE_SIZE, yf1);
	  bb = ART_UTA_BBOX_CONS (0, yf0, ART_UTILE_SIZE, yf1);
	  for (x = 1; x < width - 1; x++)
	    utiles[x] = bb;
	  utiles[x] = ART_UTA_BBOX_CONS (0, yf0, xf1, yf1);
	}
    }
  else
    {
      if (width == 1)
	{
	  utiles[0] = ART_UTA_BBOX_CONS (xf0, yf0, xf1, ART_UTILE_SIZE);
	  bb = ART_UTA_BBOX_CONS (xf0, 0, xf1, ART_UTILE_SIZE);
	  for (y = 1; y < height - 1; y++)
	    utiles[y] = bb;
	  utiles[y] = ART_UTA_BBOX_CONS (xf0, 0, xf1, yf1);
	}
      else
	{
	  utiles[0] =
	    ART_UTA_BBOX_CONS (xf0, yf0, ART_UTILE_SIZE, ART_UTILE_SIZE);
	  bb = ART_UTA_BBOX_CONS (0, yf0, ART_UTILE_SIZE, ART_UTILE_SIZE);
	  for (x = 1; x < width - 1; x++)
	    utiles[x] = bb;
	  utiles[x] = ART_UTA_BBOX_CONS (0, yf0, xf1, ART_UTILE_SIZE);
	  ix = width;
	  for (y = 1; y < height - 1; y++)
	    {
	      utiles[ix++] =
		ART_UTA_BBOX_CONS (xf0, 0, ART_UTILE_SIZE, ART_UTILE_SIZE);
	      bb = ART_UTA_BBOX_CONS (0, 0, ART_UTILE_SIZE, ART_UTILE_SIZE);
	      for (x = 1; x < width - 1; x++)
		utiles[ix++] = bb;
	      utiles[ix++] = ART_UTA_BBOX_CONS (0, 0, xf1, ART_UTILE_SIZE);
	    }
	  utiles[ix++] = ART_UTA_BBOX_CONS (xf0, 0, ART_UTILE_SIZE, yf1);
	  bb = ART_UTA_BBOX_CONS (0, 0, ART_UTILE_SIZE, yf1);
	  for (x = 1; x < width - 1; x++)
	    utiles[ix++] = bb;
	  utiles[ix++] = ART_UTA_BBOX_CONS (0, 0, xf1, yf1);
	}
    }
  return uta;
}
