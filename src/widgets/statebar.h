/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#ifndef WIDGETS_STATEBAR_H_INCLUDED
#define WIDGETS_STATEBAR_H_INCLUDED

#include "jinete/jbase.h"

#include "core/color.h"

typedef struct Progress
{
  JWidget statusbar;
  float pos;
} Progress;

typedef struct StatusBar
{
  JWidget widget;
  int timeout;

  /* progress bar */
  JList progress;

  /* box of main commands */
  JWidget commands_box;
  JWidget b_layer;			/* layer button */
  JWidget slider;			/* opacity slider */
  JWidget b_first;			/* go to first frame */
  JWidget b_prev;			/* go to previous frame */
  JWidget b_play;			/* play animation */
  JWidget b_next;			/* go to next frame */
  JWidget b_last;			/* go to last frame */

  /* tip window */
  JWidget tipwindow;
} StatusBar;

/* statusbar */

JWidget statusbar_new();
int statusbar_type();

StatusBar *statusbar_data(JWidget widget);

void statusbar_set_text(JWidget widget, int msecs, const char *format, ...);
void statusbar_show_tip(JWidget widget, int msecs, const char *format, ...);
void statusbar_show_color(JWidget widget, int msecs, int imgtype, color_t color);

/* progress */

Progress *progress_new(JWidget widget);
void progress_free(Progress *progress);
void progress_update(Progress *progress, float progress_pos);

#endif
