/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Original source code from mapgen 1.1, Copyright by Johan Halmén and
   Anders "Trezker" Andersson

   http://edu.loviisa.fi/~lg/jh/mapgen/
   http://www.angelfire.com/art/dumlesoft/Projects.html
 */

#include "config.h"

#include <stdlib.h>

#include "core/app.h"
#include "raster/image.h"
#include "widgets/statebar.h"

static int Minus (int j, int i, int size);
static int Plus (int j, int i, int size);
static float FRandom (float amount);

void mapgen (Image *image, int seed, float fractal_factor)
{
  Progress *progress = NULL;
  float **map;
  float amount = 128;
  float min, max;
  int i, j, k;
  int size = 256;

  srand (seed);

  /**********************************************************************/
  /* create the map */

  map = malloc (sizeof(float *) * size);
  for (i = 0; i < size; i++)
    map[i] = malloc(sizeof(float) * size);

  /* Do the corners */
  map[0][0] = 0;		/* map[0][0] = FRandom(amount); */
  map[size-1][0] = map[0][0];
  map[size-1][size-1] = map[0][0];
  map[0][size-1] = map[0][0];
  amount /= fractal_factor;

  if (app_get_status_bar())
    progress = progress_new(app_get_status_bar());

  for (i=128; i>0; i/=2) {
    /* This is the square phase */
    for (j=i; j<size; j+=2*i)
      for (k=i; k<size; k+=2*i) {
	map[j][k] = (map[Minus(j, i, size)][Minus(k, i, size)] +
		     map[Minus(j, i, size)][Plus(k, i, size)] +
		     map[Plus(j, i, size)][Plus(k, i, size)] +
		     map[Plus(j, i, size)][Minus(k, i, size)]) / 4.0
	  + FRandom(amount);
      }

    amount /= fractal_factor;

    /* This is the diamond phase */
    for (j=0; j<size; j+=i)
      for (k=0; k<size; k+=i)
	if (((j+k)/i) & 1) {
	  /* In this phase we might fall off the edge when we count
	     the average of neighbours Minus and Plus take care of
	     that */
	  map[j][k] = (map[Minus(j, i, size)][k] +
		       map[j][Minus(k, i, size)] +
		       map[Plus(j, i, size)][k] +
		       map[j][Plus(k, i, size)]) / 4.0
	    + FRandom(amount);
	}

    amount /= fractal_factor;

    if (progress)
      progress_update(progress, (128.0f-i) / 128.0f);
  }

  if (progress)
    progress_free(progress);

  /**********************************************************************/
  /* Copy the map to the image */

  min = max = 0;

  for (i=0; i<size; i++)
    for (j=0; j<size; j++) {
      if (min > map[i][j]) min = map[i][j];
      if (max < map[i][j]) max = map[i][j];
    }

  for (i=0; i<size; i++)
    for (j=0; j<size; j++) {
      k = (int)((map[i][j] - min)/(max - min) * 256);
      if (k > 255)
	k = 255;
      image_putpixel(image, i, j, k);
    }

  for (i=0; i<size; i++)
    jfree(map[i]);
  jfree(map);
}

/* Handles wrapping when seeking neighbours */
static int Minus (int j, int i, int size)
{
  int r = j - i;
  if (r >= 0)
    return r;
  else
    return r + size;
}

/* Me too */
static int Plus (int j, int i, int size)
{
  int r = j + i;
  if (r < size)
    return r;
  else
    return r - size;
}

static float FRandom (float amount)
{
  return amount * (((float)rand()) / RAND_MAX - 0.5);
}

