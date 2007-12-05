/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2005, 2007  David A. Capello
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

#ifndef USE_PRECOMPILED_HEADER

#include <allegro.h>

#include "jinete/jmanager.h"
#include "jinete/jsystem.h"

#include "core/core.h"
#include "modules/gui.h"

#endif

void dialogs_screen_saver(void)
{
  int r_value, g_value, b_value;
  int r_delta, g_delta, b_delta;
  V3D_f v1, v2, v3;
  PALETTE palette;
  PALETTE backup;
  int c;

  if (!is_interactive())
    return;

  /* hide the mouse */
  jmouse_set_cursor(JI_CURSOR_NULL);

  /* get the current color palette */
  get_palette(backup);

  /* flush drawing messages (useful when we are using double-buffering) */
  gui_feedback();

  /* clear the screen */
  clear(ji_screen);

  /* start the values */
  r_value = (rand() & 0x3F);
  g_value = (rand() & 0x3F);
  b_value = (rand() & 0x3F);
  r_delta = (rand() & 5) + 1;
  g_delta = (rand() & 5) + 1;
  b_delta = (rand() & 5) + 1;

  /* clear the keyboard buffer */
  clear_keybuf ();

  do {
    /* generate the palette */
    palette[0].r = palette[0].g = palette[0].b = 0;
    for (c=1; c<PAL_SIZE; c++) {
      palette[c].r = (r_value & 0x3F) * c / (PAL_SIZE-1);
      palette[c].g = (g_value & 0x3F) * c / (PAL_SIZE-1);
      palette[c].b = (b_value & 0x3F) * c / (PAL_SIZE-1);
    }

    /* update values */
    r_value += r_delta;
    g_value += g_delta;
    b_value += b_delta;

    if ((r_value < 0) || (r_value > 0x3F)) {
      r_value -= r_delta;
      r_delta = -SGN(r_delta) * ((rand() & 5) + 1);
    }
    if ((g_value < 0) || (g_value > 0x3F)) {
      g_value -= g_delta;
      g_delta = -SGN(g_delta) * ((rand() & 5) + 1);
    }
    if ((b_value < 0) || (b_value > 0x3F)) {
      b_value -= b_delta;
      b_delta = -SGN(b_delta) * ((rand() & 5) + 1);
    }

    /* vertices */
    v1.x = (rand()%(JI_SCREEN_W+64)) - 32;
    v1.y = (rand()%(JI_SCREEN_H+64)) - 32;
    v2.x = (rand()%(JI_SCREEN_W+64)) - 32;
    v2.y = (rand()%(JI_SCREEN_H+64)) - 32;
    v3.x = (rand()%(JI_SCREEN_W+64)) - 32;
    v3.y = (rand()%(JI_SCREEN_H+64)) - 32;

    /* vertical-retrace */
    vsync();
    set_palette_range(palette, 0, PAL_SIZE-1, FALSE);

    /* colors */
    v1.c = palette_color[rand()%256];
    v2.c = palette_color[rand()%256];
    v3.c = palette_color[rand()%256];

    /* draw a triangle */
    triangle3d_f(ji_screen,
		 (bitmap_color_depth(ji_screen) == 8)?
		 POLYTYPE_GCOL: POLYTYPE_GRGB, NULL, &v1, &v2, &v3);

    /* poll GUI */
    jmouse_poll();
    gui_feedback();
  } while ((!keypressed()) && (!jmouse_b(0)));

  /* clear the screen */
  clear(ji_screen);

  /* restore the color palette */
  set_palette(backup);

  /* wait while the user has pushed some mouse button */
  do {
    jmouse_poll();
    gui_feedback();
  } while (jmouse_b(0));

  jmanager_refresh_screen();
  jmouse_set_cursor(JI_CURSOR_NORMAL);

  /* clear again the keyboard buffer */
  clear_keybuf();
}
