/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

#include "jinete.h"

#include "console/console.h"
#include "core/app.h"
#include "core/cfg.h"
#include "core/core.h"
#include "dialogs/gfxsel.h"
#include "dialogs/options.h"
#include "modules/gui.h"
#include "modules/palette.h"

#endif

/*
  set_gfx_mode (card-name, width, height, bpp)
    card = interactive
           autodetect (default)
           autodetect-fullscreen
           autodetect-windowed
*/
int set_gfx(const char *_card, int w, int h, int depth)
{
  int card, old_card, old_w, old_h, old_depth;

  /* in non-interactive do nothing */
  if (!is_interactive())
    return FALSE;

  /* get the active status */
  old_card  = gfx_driver->id;
  old_w     = SCREEN_W;
  old_h     = SCREEN_H;
  old_depth = bitmap_color_depth(screen);

  /* select mode */
  if (ustricmp(_card, "interactive") == 0) {
    /* default values */
    card = old_card;
    w = old_w;
    h = old_h;
    depth = old_depth;

    /* select the new graphics mode */
    if (gfxmode_select(&card, &w, &h, &depth) == 0)
      return FALSE;
  }
  /* get arguments */
  else {
    if (ustricmp(_card, "autodetect") == 0)
      card = GFX_AUTODETECT;
    else if (ustricmp(_card, "autodetect-fullscreen") == 0)
      card = GFX_AUTODETECT_FULLSCREEN;
    else if (ustricmp(_card, "autodetect-windowed") == 0)
      card = GFX_AUTODETECT_WINDOWED;
    /* invalid `card' parameter */
    else {
      console_printf(_("Error in `gfx_mode', invalid card: \"%s\"\n"), _card);
      return FALSE;
    }

    if ((w < 1) || (h < 1)) {
      console_printf(_("Error in `set_gfx', invalid screen size: %dx%d\n"), w, h);
      return FALSE;
    }

    if ((depth != 8) && (depth != 15) && (depth != 16) &&
                        (depth != 24) && (depth != 32)) {
      console_printf(_("Error in `gfx_mode', invalid color depth: %d\n"), depth);
      return FALSE;
    }
  }

  /* try change the new graphics mode */
  set_color_depth(depth);
  if(set_gfx_mode(card, w, h, 0, 0) < 0) {
    /* error!, well, we need to return to the old graphics mode */
    set_color_depth(old_depth);
    if(set_gfx_mode(old_card, old_w, old_h, 0, 0) < 0) {
      /* oh no! more errors!, we can't restore the old graphics mode! */
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      console_printf(_("FATAL ERROR: Unable to restore the old graphics mode!\n"));
      app_exit();
      exit(1);
    }
    /* only print a message of the old error */
    else {
      gui_setup_screen();

      /* set to a black palette */
      set_current_palette(black_palette, TRUE);

      /* restore palette all screen stuff */
      app_refresh_screen();

      console_printf(_("Error setting graphics mode: %dx%d %d bpp\n"),
		     w, h, depth);
    }
  }
  /* the new graphics mode is working */
  else {
    gui_setup_screen();

    /* set to a black palette */
    set_current_palette(black_palette, TRUE);

    /* restore palette all screen stuff */
    app_refresh_screen();
  }
  
  /* setup mouse */
/*   show_mouse(ji_screen); */
  _setup_mouse_speed();

  /* redraw top window */
  if (app_get_top_window()) {
    jwindow_remap(app_get_top_window());
    jmanager_refresh_screen();
  }

  return TRUE;
}
