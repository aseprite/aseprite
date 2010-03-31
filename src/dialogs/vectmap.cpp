/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "config.h"

#include <allegro.h>
#include <cmath>

#include "core/core.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "util/misc.h"

static void project(Image *image, int x, int y, double dmax, double *out_x, double *out_y)
{
  int center_x = image->w/2;	/* center point */
  int center_y = image->h/2;
  double u = (x - center_x);	/* vector from center to current point */
  double v = (y - center_y);
  int r = MIN(image->w, image->h)/2;  /* radius */
  double d = std::sqrt(u*u + v*v);    /* distance from center */
  double a = std::atan2(-v, u);	      /* angle with center */

#if 1
  /* sphere */
  double s = std::sin(PI*MIN(d, r)/r); /* scale factor for effect */
/*   double s = std::cos(PI/2*MIN(d, r)/r); /\* scale factor for effect *\/ */
  double howmuch = 32;
/*   *out_x = s * (std::cos(a)*howmuch*(dmax-d)/dmax); */
/*   *out_y = s * (std::sin(a)*howmuch*(dmax-d)/dmax); */
  *out_x = s * (std::cos(a)*howmuch*d/dmax);
  *out_y = s * (std::sin(a)*howmuch*d/dmax);

#elif 0

  /* rotation CCW */
  *out_x = (u + v);
  *out_y = (u - v);

#elif 1

  /* twist */
/*   double s = sin(PI*4*MIN(d, r)/r); /\* scale factor for effect *\/ */
/*   *out_x = s * (cos(a)*8); */
/*   *out_y = s * (sin(a)*8); */
  /* double twist_angle = 2*PI; */

/*   *out_x = cos(PI/2*MIN(d,r)/r) * (d); */
/*   *out_y = cos(PI/2*MIN(d,r)/r) * (d); */

#endif
}

/* original code from 2xSaI by Derek Liauw Kie Fa and Robert J
   Ohannessian */
static unsigned long bilinear4 (unsigned long A, unsigned long B,
				unsigned long C, unsigned long D,
				fixed x, fixed y)
{
  unsigned long xy, areaA, areaB, areaC, areaD;
  unsigned long res_r, res_g, res_b, res_a;

  x = (x >> 11) & 0x1f;
  y = (y >> 11) & 0x1f;
  xy = (x*y) >> 5;

/*   A = (A & redblueMask) | ((A & greenMask) << 16); */
/*   B = (B & redblueMask) | ((B & greenMask) << 16); */
/*   C = (C & redblueMask) | ((C & greenMask) << 16); */
/*   D = (D & redblueMask) | ((D & greenMask) << 16); */

  areaA = 0x20 + xy - x - y;
  areaB = x - xy;
  areaC = y - xy;
  areaD = xy;

#define CHANNEL(channel)					\
  res_##channel = ((areaA * _rgba_get##channel(A)) +		\
		   (areaB * _rgba_get##channel(B)) +		\
		   (areaC * _rgba_get##channel(C)) +		\
		   (areaD * _rgba_get##channel(D))) >> 5

  CHANNEL(r);
  CHANNEL(g);
  CHANNEL(b);
  CHANNEL(a);

  return _rgba(res_r, res_g, res_b, res_a);
}

static int image_getpixel4 (Image *image, double x, double y)
{
  int x1, y1, x2, y2, a, b, c, d;

  x1 = floor(x);
  y1 = floor(y);
  x2 = ceil(x);
  y2 = ceil(y);

  a = image_getpixel(image, MID(0, x1, image->w-1), MID(0, y1, image->h-1));
  b = image_getpixel(image, MID(0, x2, image->w-1), MID(0, y1, image->h-1));
  c = image_getpixel(image, MID(0, x1, image->w-1), MID(0, y2, image->h-1));
  d = image_getpixel(image, MID(0, x2, image->w-1), MID(0, y2, image->h-1));

  return bilinear4(a, b, c, d, ftofix(x), ftofix(y));
}

void dialogs_vector_map(Sprite* sprite)
{
#define PROJECT()	project(image, x, y, dmax, &u, &v)

  Image *image, *image_copy;
  double u, v, dmax;
  int c, x, y;

  if (!is_interactive () || !sprite)
    return;

  image = sprite->getCurrentImage();
  if (!image)
    return;

  image_copy = image_new_copy(image);
  if (!image_copy)
    return;

  /* undo stuff */
  if (undo_is_enabled(sprite->getUndo()))
    undo_image(sprite->getUndo(), image, 0, 0, image->w, image->h);

  dmax = std::sqrt(static_cast<double>(image->w/2*image->w/2 + image->h/2*image->h/2));
  for (y=0; y<image->h; y++) {
    for (x=0; x<image->w; x++) {
      PROJECT();
      c = image_getpixel4 (image_copy, x-u, y+v);
      image_putpixel(image, x, y, c);
    }
  }

  image_free(image_copy);

  update_screen_for_sprite(sprite);
}
