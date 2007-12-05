/* ASE - Allegro Sprite Editor
 * Copyright (C) 2007  David A. Capello
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

#include "jinete/jinete.h"

#include "commands/commands.h"
#include "console/console.h"
#include "core/app.h"
/* #include "core/cfg.h" */
/* #include "core/core.h" */
/* #include "dialogs/options.h" */
#include "modules/gui.h"
#include "modules/palette.h"

#endif

#define DEPTH_TO_INDEX(bpp)			\
  ((bpp == 8)? 0:				\
   (bpp == 15)? 1:				\
   (bpp == 16)? 2:				\
   (bpp == 24)? 3:				\
   (bpp == 32)? 4: -1)

#define INDEX_TO_DEPTH(index)			\
  ((index == 0)? 8:				\
   (index == 1)? 15:				\
   (index == 2)? 16:				\
   (index == 3)? 24:				\
   (index == 4)? 32: -1)

static int new_card, new_w, new_h, new_depth, new_scaling;
static int old_card, old_w, old_h, old_depth, old_scaling;

static void show_dialog(void);
static void try_new_gfx_mode(void);

static void cmd_configure_screen_execute(const char *argument)
{
  /* get the active status */
  old_card    = gfx_driver->id;
  old_w       = SCREEN_W;
  old_h       = SCREEN_H;
  old_depth   = bitmap_color_depth(screen);
  old_scaling = get_screen_scaling();

  /* default values */
  new_card = old_card;
  new_w = old_w;
  new_h = old_h;
  new_depth = old_depth;

  show_dialog();
}

static void show_dialog(void)
{
  JWidget window, resolution, color_depth, pixel_scale, fullscreen;
  char buf[512];

  window = load_widget("confscr.jid", "configure_screen");
  if (!window)
    return;

  if (!get_widgets(window,
		   "resolution", &resolution,
		   "color_depth", &color_depth,
		   "pixel_scale", &pixel_scale,
		   "fullscreen", &fullscreen, NULL)) {
    jwidget_free(window);
    return;
  }

  jcombobox_add_string(resolution, "320x200");
  jcombobox_add_string(resolution, "320x240");
  jcombobox_add_string(resolution, "640x400");
  jcombobox_add_string(resolution, "640x480");
  jcombobox_add_string(resolution, "800x600");
  jcombobox_add_string(resolution, "1024x768");

  jcombobox_add_string(color_depth, _("8 bpp (256 colors)"));
  jcombobox_add_string(color_depth, _("15 bpp (32K colors)"));
  jcombobox_add_string(color_depth, _("16 bpp (64K colors)"));
  jcombobox_add_string(color_depth, _("24 bpp (16M colors)"));
  jcombobox_add_string(color_depth, _("32 bpp (16M colors)"));

  jcombobox_add_string(pixel_scale, "x1 (normal)");
  jcombobox_add_string(pixel_scale, "x2 (double)");
  jcombobox_add_string(pixel_scale, "x3 (big)");
  jcombobox_add_string(pixel_scale, "x4 (huge)");

  usprintf(buf, "%dx%d", old_w, old_h);
  jcombobox_select_string(resolution, buf);

  jcombobox_select_index(color_depth, DEPTH_TO_INDEX(old_depth));
  jcombobox_select_index(pixel_scale, get_screen_scaling()-1);

  if (is_windowed_mode())
    jwidget_deselect(fullscreen);
  else
    jwidget_select(fullscreen);

  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == jwidget_find_name(window, "ok")) {
    int depth_index;
    char *xbuf;

    ustrcpy(buf, jcombobox_get_selected_string(resolution));
    new_w = ustrtol(buf, &xbuf, 10);
    new_h = ustrtol(xbuf+1, NULL, 10);

    new_card = jwidget_is_selected(fullscreen) ? GFX_AUTODETECT_FULLSCREEN:
						 GFX_AUTODETECT_WINDOWED;

    depth_index = jcombobox_get_selected_index(color_depth);
    new_depth = INDEX_TO_DEPTH(depth_index);

    new_scaling = jcombobox_get_selected_index(pixel_scale)+1;

    try_new_gfx_mode();
  }

  jwidget_free(window);
}

static void try_new_gfx_mode(void)
{
  /* try change the new graphics mode */
  set_color_depth(new_depth);
  set_screen_scaling(new_scaling);
  if (set_gfx_mode(new_card, new_w, new_h, 0, 0) < 0) {
    /* error!, well, we need to return to the old graphics mode */
    set_color_depth(old_depth);
    set_screen_scaling(old_scaling);
    if (set_gfx_mode(old_card, old_w, old_h, 0, 0) < 0) {
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
		     new_w, new_h, new_depth);
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
  _setup_mouse_speed();

  /* redraw top window */
  if (app_get_top_window()) {
    jwindow_remap(app_get_top_window());
    jmanager_refresh_screen();
  }
}

Command cmd_configure_screen = {
  CMD_CONFIGURE_SCREEN,
  NULL,
  NULL,
  cmd_configure_screen_execute,
  NULL
};
