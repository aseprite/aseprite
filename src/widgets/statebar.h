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

#ifndef WIDGETS_STATEBAR_H
#define WIDGETS_STATEBAR_H

#include "jinete/base.h"

typedef struct StatusBar
{
  JWidget widget;
  int timeout;
  unsigned restore : 1;		/* return from commands mode */

  /* progress bar */
  int nprogress;
  struct {
    int max;
    int pos;
  } progress[2];

  /* box of main commands */
  JWidget commands_box;
  JWidget b_layer;			/* layer button */
  JWidget slider;			/* opacity slider */
  JWidget b_first;			/* go to first frame */
  JWidget b_prev;			/* go to previous frame */
  JWidget b_play;			/* play animation */
  JWidget b_next;			/* go to next frame */
  JWidget b_last;			/* go to last frame */
} StatusBar;

JWidget status_bar_new (void);
int status_bar_type (void);

StatusBar *status_bar_data (JWidget status_bar);

void status_bar_set_text (JWidget status_bar, int msecs, const char *format, ...);

void status_bar_do_progress (JWidget status_bar, int progress);
void status_bar_add_progress (JWidget status_bar, int max);
void status_bar_del_progress (JWidget status_bar);

void status_bar_update (JWidget status_bar);

#endif /* WIDGETS_STATEBAR_H */
