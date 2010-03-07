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

#include "raster/raster.h"

static void draw_dirty(int x1, int y, int y2, void *data)
{
  image_hline(reinterpret_cast<Image*>(data), x1, y, y2, rand() % 256);
}

int main (int argc, char *argv[])
{
  Pen* pen1;
  Pen* pen2;
  Dirty* dirty;
  Image* image;
  BITMAP* bmp;
  int redraw;
  int mx, my, mb, ox, oy, ob;
  int c;

  allegro_init();
  set_color_depth(8);
  set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0);
  install_timer();
  install_keyboard();
  install_mouse();

  gfxobj_init();

  bmp = create_bitmap(320, 240);
  redraw = true;

  image = image_new(IMAGE_INDEXED, bmp->w, bmp->h);
  dirty = dirty_new(image, 0, 0, image->w-1, image->h-1, false);
  pen1 = pen_new();
  pen2 = pen_new();

  pen_set_size(pen1, 1);
  pen_set_size(pen2, 8);

  image_clear(image, 0);
  for (c=128; c>0; c-=8)
    image_ellipse(image,
		  image->w/2-c, image->h/2-c/2,
		  image->w/2+c, image->h/2+c/2, c);

  ox = mouse_x/2;
  oy = mouse_y/2;

  show_mouse(screen);

  do {
    poll_mouse();
    mx = mouse_x/2;
    my = mouse_y/2;
    mb = mouse_b;
    if (mx != ox || my != oy || mb != ob) {
      poll_mouse();
      mx = mouse_x/2;
      my = mouse_y/2;
      mb = mouse_b;

      if (mb) {
	dirty_line_pen(dirty, (mb & 2) ? pen2: pen1,
		       ox, oy, mx, my);
	redraw = true;
      }

      dirty_save_image_data(dirty);
      algo_dirty(dirty, image, (AlgoHLine)draw_dirty);

      ox = mx;
      oy = my;
      ob = mb;
    }

    /* with R restore the image */
    if (key[KEY_R]) {
      dirty_restore_image_data(dirty);
      dirty_free(dirty);
      dirty = dirty_new(image, 0, 0, image->w-1, image->h-1, false);
      while (key[KEY_R]);
      redraw = true;
    }

    /* with S swap */
    if (key[KEY_S]) {
      dirty_swap(dirty);
      while (key[KEY_S]);
      redraw = true;
    }

    if (redraw) {
      redraw = false;

      scare_mouse();
      clear(bmp);
      image_to_allegro(image, bmp, 0, 0);
      text_mode(0);
      textout(bmp, font, "R:restore image", 0, 0, 15);
      stretch_blit(bmp, screen, 0, 0, bmp->w, bmp->h, 0, 0, SCREEN_W, SCREEN_H);
      unscare_mouse();
    }
  } while (!key[KEY_ESC]);

  gfxobj_exit();
  return 0;
}

END_OF_MAIN();
