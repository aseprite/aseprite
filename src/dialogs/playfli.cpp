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
#include <stdio.h>
#include <string.h>

#include "jinete/jmanager.h"
#include "jinete/jsystem.h"

#include "core/core.h"
#include "file/fli/fli.h"
#include "modules/gui.h"

static bool my_callback();
static void my_play_fli(const char *filename, bool loop, bool fullscreen,
			bool (*callback)());

void play_fli_animation(const char *filename, bool loop, bool fullscreen)
{
  if (is_interactive()) {
    PALETTE backup;

    jmanager_free_mouse();

    /* hide the mouse */
    jmouse_hide();

    /* get the current color palette */
    get_palette(backup);

    /* clear the screen */
    clear(ji_screen);

    /* clear the keyboard buffer */
    clear_keybuf();

    /* play the fli */
    my_play_fli(filename, loop, fullscreen, my_callback);
    /* play_fli(filename, ji_screen, loop, my_callback); */

    /* clear the screen */
    clear(ji_screen);

    /* restore the color palette */
    set_palette(backup);

    /* show the mouse cursor */
    jmouse_show();

    /* wait while the user has pushed some mouse button */
    do {
      jmouse_poll();
    } while (jmouse_b(0));

    /* clear again the keyboard buffer */
    clear_keybuf();

    jmanager_refresh_screen();
  }
}

static bool my_callback()
{
  jmouse_poll();

  return (keypressed() || jmouse_b(0));
}

/**********************************************************************/
/* my_play_fli */

static int speed_timer;

static void speed_timer_callback()
{
  speed_timer++;
}

END_OF_STATIC_FUNCTION(speed_timer_callback);

static void my_play_fli(const char *filename, bool loop, bool fullscreen,
			bool (*callback)())
{
  unsigned char cmap[768];
  unsigned char omap[768];
  s_fli_header fli_header;
  BITMAP *bmp, *old;
  int x, y, w, h;
  PALETTE pal;
  int frpos;
  int done;
  int c, i;
  FILE *f;

  /* open the file to read in binary mode */
  f = fopen(filename, "rb");
  if (!f)
    return;

  /* read the header */
  fli_read_header(f, &fli_header);
  fseek(f, 128, SEEK_SET);

  bmp = create_bitmap_ex(8, fli_header.width, fli_header.height);
  old = create_bitmap_ex(8, fli_header.width, fli_header.height);

  /* stretch routine doesn't support bitmaps of different color depths */
  if (bitmap_color_depth(ji_screen) != 8)
    fullscreen = false;

  w = fli_header.width;
  h = fli_header.height;

  if (fullscreen) {
    double scale;

    if (JI_SCREEN_W-bmp->w > JI_SCREEN_H-bmp->h)
      scale = (double)JI_SCREEN_W / (double)w;
    else
      scale = (double)JI_SCREEN_H / (double)h;

    w = (double)w * (double)scale;
    h = (double)h * (double)scale;
  }

  x = JI_SCREEN_W/2 - w/2;
  y = JI_SCREEN_H/2 - h/2;

  LOCK_VARIABLE(speed_timer);
  LOCK_FUNCTION(speed_timer_callback);

  speed_timer = 0;
  install_int_ex(speed_timer_callback, MSEC_TO_TIMER(fli_header.speed));

  frpos = 0;
  done = false;

  while (!done) {
    /* read the frame */
    fli_read_frame(f, &fli_header,
		   (unsigned char *)old->dat, omap,
		   (unsigned char *)bmp->dat, cmap);

    if ((!frpos) || (memcmp (omap, cmap, 768) != 0)) {
      for (c=i=0; c<256; c++) {
	pal[c].r = cmap[i++]>>2;
	pal[c].g = cmap[i++]>>2;
	pal[c].b = cmap[i++]>>2;
      }
      set_palette(pal);
      memcpy(omap, cmap, 768);
    }

    if (fullscreen)
      stretch_blit(bmp, ji_screen,
		   0, 0, fli_header.width, fli_header.height, x, y, w, h);
    else
      blit(bmp, ji_screen, 0, 0, x, y, w, h);

    jmanager_refresh_screen();
    gui_feedback();

    do {
      if ((*callback) ()) {
	done = true;
	break;
      }
    } while (speed_timer <= 0);

    if ((++frpos) >= fli_header.frames) {
      if (!loop)
	break;
      else {
	fseek(f, 128, SEEK_SET);
	frpos = 0;
      }
    }

    blit(bmp, old, 0, 0, 0, 0, fli_header.width, fli_header.height);
    speed_timer--;
  }

  destroy_bitmap(bmp);
  destroy_bitmap(old);

  fclose(f);
}

