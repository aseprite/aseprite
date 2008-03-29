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

#include "config.h"
#include "test/test.h"

#include <errno.h>

#include "raster/blend.h"
#include "raster/image.h"

#include <winalleg.h>

int real_rgba_blend_normal(int back, int front, int opacity)
{
  register int t;

  if ((back & 0xff000000) == 0) {
    return
      (front & 0xffffff) |
      (INT_MULT(_rgba_geta(front), opacity, t) << _rgba_a_shift);
  }
  else if ((front & 0xff000000) == 0) {
    return back;
  }
  else {
    int B_r, B_g, B_b, B_a;
    int F_r, F_g, F_b, F_a;
    int D_r, D_g, D_b, D_a;
    
    B_r = _rgba_getr(back);
    B_g = _rgba_getg(back);
    B_b = _rgba_getb(back);
    B_a = _rgba_geta(back);

    F_r = _rgba_getr(front);
    F_g = _rgba_getg(front);
    F_b = _rgba_getb(front);
    F_a = _rgba_geta(front);
    F_a = INT_MULT(F_a, opacity, t);

    D_a = B_a + F_a - INT_MULT(B_a, F_a, t);
    D_r = B_r + (F_r-B_r) * F_a / D_a;
    D_g = B_g + (F_g-B_g) * F_a / D_a;
    D_b = B_b + (F_b-B_b) * F_a / D_a;

    return _rgba(D_r, D_g, D_b, D_a);
  }
}

/* retorna "a - b" en segundos */
double performancecounter_diff(LARGE_INTEGER *a, LARGE_INTEGER *b)
{
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  return (double)(a->QuadPart - b->QuadPart) / (double)freq.QuadPart;
}

static void test_rgba_blend_normal(void)
{
  int c, x, y, z;
  int shift[4] = { 0, 8, 16, 24 };

  for (c=0; c<4; ++c)
    for (x=0; x<256; ++x)
      for (y=0; y<256; ++y)
	for (z=0; z<256; ++z) {
	  if (_rgba_blend_normal(x<<shift[c], y<<shift[c], z) !=
	      real_rgba_blend_normal(x<<shift[c], y<<shift[c], z)) {
	    printf("_rgba_blend_normal(%08x, %08x, %02x)\n", x<<shift[c], y<<shift[c], z);
	    assert(FALSE);
	  }
	  if (_rgba_blend_normal_sse(x<<shift[c], y<<shift[c], z) !=
	      real_rgba_blend_normal(x<<shift[c], y<<shift[c], z)) {
	    printf("_rgba_blend_normal_sse(%08x, %08x, %02x)\n", x<<shift[c], y<<shift[c], z);
	    assert(FALSE);
	  }
	}

  /* _mm_empty(); */

#if 1
  {
  LARGE_INTEGER t_ini, t_fin;
  double secs;

  QueryPerformanceCounter(&t_ini);
  for (c=0; c<10; ++c)
  for (x=0; x<256; ++x)
    for (y=0; y<256; ++y)
      for (z=0; z<256; ++z)
	_rgba_blend_normal(x, y, z);
  QueryPerformanceCounter(&t_fin);
  
  secs = performancecounter_diff(&t_fin, &t_ini);
  printf("current: %.16g milliseconds\n", secs * 1000.0 / (256.*256.*256.*10.));

  QueryPerformanceCounter(&t_ini);
  for (c=0; c<10; ++c)
  for (x=0; x<256; ++x)
    for (y=0; y<256; ++y)
      for (z=0; z<256; ++z)
	_rgba_blend_normal_sse(x, y, z);
  QueryPerformanceCounter(&t_fin);
  
  secs = performancecounter_diff(&t_fin, &t_ini);
  printf("withsse: %.16g milliseconds\n", secs * 1000.0 / (256.*256.*256.*10.));

  }
#endif
}

int main(int argc, char *argv[])
{
  test_init();
  test_rgba_blend_normal();
  return test_exit();
}

END_OF_MAIN();
