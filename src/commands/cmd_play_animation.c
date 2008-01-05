/* ASE - Allegro Sprite Editor
 * Copyright (C) 2008  David A. Capello
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
#include "modules/editors.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "modules/sprites.h"
#include "raster/sprite.h"
#include "widgets/editor.h"

#endif

static int speed_timer;

static void speed_timer_callback(void)
{
  speed_timer++;
}

END_OF_STATIC_FUNCTION(speed_timer_callback);

static bool cmd_play_animation_enabled(const char *argument)
{
  return current_sprite != NULL;
}

static void cmd_play_animation_execute(const char *argument)
{
  Sprite *sprite = current_sprite;
  int old_frame, msecs;
  bool done = FALSE;

  if (sprite->frames < 2)
    return;

  jmouse_hide();

  old_frame = sprite->frame;

  LOCK_VARIABLE(speed_timer);
  LOCK_FUNCTION(speed_timer_callback);

  clear_keybuf();

  /* clear all the screen */
  clear_bitmap(ji_screen);

  /* do animation */
  speed_timer = 0;
  while (!done) {
    msecs = sprite_get_frlen(sprite, sprite->frame);
    install_int_ex(speed_timer_callback, MSEC_TO_TIMER(msecs));

    set_palette(sprite_get_palette(sprite, sprite->frame));
    editor_draw_sprite_safe(current_editor, 0, 0, sprite->w, sprite->h);

    do {
      poll_mouse();
      poll_keyboard();
      if (keypressed() || mouse_b)
	done = TRUE;
      gui_feedback();
    } while (!done && (speed_timer <= 0));

    if (!done) {
      sprite->frame++;
      if (sprite->frame >= sprite->frames)
	sprite->frame = 0;

      speed_timer--;
    }
    gui_feedback();
  }

  /* if right-click or ESC */
  if (mouse_b == 2 || (keypressed() && (readkey()>>8) == KEY_ESC))
    /* return to the old frame position */
    sprite->frame = old_frame;

  /* refresh all */
  set_current_palette(sprite_get_palette(sprite, sprite->frame), TRUE);
  jmanager_refresh_screen();
  gui_feedback();

  while (mouse_b)
    poll_mouse();

  clear_keybuf();
  remove_int(speed_timer_callback);

  jmouse_show();
}

Command cmd_play_animation = {
  CMD_PLAY_ANIMATION,
  cmd_play_animation_enabled,
  NULL,
  cmd_play_animation_execute,
};
