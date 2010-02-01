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

#include "raster/image.h"
#include "raster/mask.h"

int main(int argc, char *argv[])
{
  Mask *mask = NULL;
  int redraw;

  allegro_init();
  set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0);
  install_timer();
  install_keyboard();
  install_mouse();

  set_mouse_sprite_focus(0, 0);
  show_mouse(screen);

  redraw = true;
  do {
    if (mouse_b) {
      int xbeg, ybeg, xend, yend, xold, yold;
      int first = mouse_b & 1;

      xbeg = xend = mouse_x;
      ybeg = yend = mouse_y;
      xold = yold = -1;

      while (mouse_b) {
	xend = mouse_x;
	yend = mouse_y;
	if ((xend != xold) || (yend != yold)) {
	  xend = mouse_x;
	  yend = mouse_y;

	  scare_mouse();

	  if (xold >= 0) {
	    xor_mode (true);
	    rect (screen, xbeg, ybeg, xold, yold, 0xff);
	    xor_mode (false);
	  }

	  xor_mode (true);
	  rect (screen, xbeg, ybeg, xold = xend, yold = yend, 0xff);
	  xor_mode (false);

	  unscare_mouse();
	}
      }

      if (!mask)
	mask = mask_new ();

      if (key_shifts & KB_ALT_FLAG) {
	if (first & 1)
	  mask_replace (mask,
			MIN (xbeg, xend),
			MIN (ybeg, yend),
			MAX (xbeg, xend) - MIN (xbeg, xend) + 1,
			MAX (ybeg, yend) - MIN (ybeg, yend) + 1);
	else
	  mask_intersect (mask,
			  MIN (xbeg, xend),
			  MIN (ybeg, yend),
			  MAX (xbeg, xend) - MIN (xbeg, xend) + 1,
			  MAX (ybeg, yend) - MIN (ybeg, yend) + 1);
      }
      else {
	if (first & 1)
	  mask_union (mask,
		      MIN (xbeg, xend),
		      MIN (ybeg, yend),
		      MAX (xbeg, xend) - MIN (xbeg, xend) + 1,
		      MAX (ybeg, yend) - MIN (ybeg, yend) + 1);
	else
	  mask_subtract (mask,
			 MIN (xbeg, xend),
			 MIN (ybeg, yend),
			 MAX (xbeg, xend) - MIN (xbeg, xend) + 1,
			 MAX (ybeg, yend) - MIN (ybeg, yend) + 1);
      }

      redraw = true;
    }

    if (key[KEY_SPACE]) {
      while (key[KEY_SPACE]);
      mask_invert (mask);
      redraw = true;
    }

    if (redraw || key[KEY_R]) {
      BITMAP *bmp;
      int x, y;

      redraw = false;

      bmp = create_bitmap (SCREEN_W, SCREEN_H);
      clear (bmp);

      if (mask) {
	if ((mask->w > 0) && (mask->h > 0))
	  rect (bmp, mask->x-1, mask->y-1,
		mask->x+mask->w, mask->y+mask->h,
		makecol (255, 255, 0));

	if (mask->bitmap) {
	  for (y=0; y<mask->h; y++)
	    for (x=0; x<mask->w; x++)
	      if (mask->bitmap)
		putpixel (bmp, mask->x+x, mask->y+y,
			  (mask->bitmap->method->getpixel (mask->bitmap, x, y))?
			  makecol (255, 255, 255): makecol (0, 0, 0));
	}
      }

      scare_mouse();
      blit(bmp, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      unscare_mouse();

      destroy_bitmap(bmp);
    }
  } while (!key[KEY_ESC]);
  return 0;
}

END_OF_MAIN();
