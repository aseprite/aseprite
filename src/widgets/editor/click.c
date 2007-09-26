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

#include <allegro/keyboard.h>

#include "jinete/rect.h"

#include "jinete/manager.h"
#include "jinete/system.h"
#include "jinete/view.h"

#include "core/cfg.h"
#include "modules/gui.h"
#include "widgets/editor.h"

#endif

/**********************************************************************/
/* editor-click interface */
/**********************************************************************/

static int click_mode;
static int click_first;

static int click_start_x;
static int click_start_y;
static int click_start_b;

static int click_last_x;
static int click_last_y;
static int click_last_b;

static int click_prev_last_b;

void editor_click_start(JWidget widget, int mode, int *x, int *y, int *b)
{
  click_mode = mode;
  click_first = TRUE;

  click_start_x = click_last_x = jmouse_x(0);
  click_start_y = click_last_y = jmouse_y(0);
  click_start_b = click_last_b = jmouse_b(0) ? jmouse_b(0): 1;

  click_prev_last_b = click_last_b;

  screen_to_editor(widget, click_start_x, click_start_y, x, y);
  *b = click_start_b;
}

void editor_click_done(JWidget widget)
{
  clear_keybuf();
}

/* returns FALSE when the user stop the click-loop: releases the
   button or press the second click (depend of the mode) */
int editor_click(JWidget widget, int *x, int *y, int *update,
		 void (*scroll_callback) (int before_change))
{
  int prev_x, prev_y;

  poll_keyboard();

  if (click_first) {
    click_first = FALSE;

    if (click_mode == MODE_CLICKANDCLICK) {
      do {
	jmouse_poll();
	gui_feedback();
      } while (jmouse_b(0));

      jmouse_set_position(click_start_x, click_start_y);
      clear_keybuf();
    }
  }

  *update = jmouse_poll();

  if (!editor_cursor_is_subpixel(widget))
    screen_to_editor(widget, click_last_x, click_last_y, &prev_x, &prev_y);

  click_prev_last_b = click_last_b;

  click_last_x = jmouse_x(0);
  click_last_y = jmouse_y(0);
  click_last_b = jmouse_b(0);

  screen_to_editor(widget, click_last_x, click_last_y, x, y);

  /* the mouse was moved */
  if (*update) {
    JWidget view = jwidget_get_view(widget);
    JRect vp = jview_get_viewport_position(view);

    /* update scroll */
    if (jmouse_control_infinite_scroll(vp)) {
      int scroll_x, scroll_y;

      if (scroll_callback)
	(*scroll_callback)(TRUE);

      /* smooth scroll movement */
      if (get_config_bool("Options", "MoveSmooth", TRUE)) {
	jmouse_set_position(MID(vp->x1+1, click_last_x, vp->x2-2),
			    MID(vp->y1+1, click_last_y, vp->y2-2));
      }
      /* this is better for high resolutions: scroll movement by big steps */
      else {
	jmouse_set_position((click_last_x != jmouse_x(0)) ?
			    (click_last_x + (vp->x1+vp->x2)/2)/2: jmouse_x(0),

			    (click_last_y != jmouse_y(0)) ?
			    (click_last_y + (vp->y1+vp->y2)/2)/2: jmouse_y(0));
      }

      jview_get_scroll(view, &scroll_x, &scroll_y);
      editor_set_scroll(widget,
			scroll_x+click_last_x-jmouse_x(0),
			scroll_y+click_last_y-jmouse_y(0), TRUE);

/*       editor_refresh_region(widget); */

      click_last_x = jmouse_x(0);
      click_last_y = jmouse_y(0);

      if (scroll_callback)
	(*scroll_callback)(FALSE);
    }

    /* if the cursor hasn't subpixel movement */
    if (!editor_cursor_is_subpixel(widget)) {
      /* check if the mouse change to other pixel of the sprite */
      *update = ((prev_x != *x) || (prev_y != *y));
    }
    else {
      /* check if the mouse change to other pixel of the screen */
      *update = ((prev_x != click_last_x) || (prev_y != click_last_y));
    }

    jrect_free(vp);
  }

  /* click-and-click mode */
  if (click_mode == MODE_CLICKANDCLICK) {
    if (click_last_b) {
      click_prev_last_b = click_last_b;

      do {
	jmouse_poll();
	gui_feedback();
      } while (jmouse_b(0));

      jmouse_set_position(click_last_x, click_last_y);
      clear_keybuf();

      return FALSE;
    }
    else {
      return TRUE;
    }
  }
  /* click-and-release mode */
  else {
    return (click_last_b) ? TRUE: FALSE;
  }
}

int editor_click_cancel(JWidget widget)
{
  return (click_start_b != click_prev_last_b);
}
