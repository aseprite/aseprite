/* ase -- allegro-sprite-editor: the ultimate sprites factory
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

#include "jinete/system.h"

#include "core/core.h"

#endif

void command_execute_screen_shot(const char *argument)
{
  int old_flag;
  char buf[512];
  PALETTE pal;
  BITMAP *bmp;
  int c;

  /* save the active flag which indicate if the mouse is freeze or not */
  old_flag = freeze_mouse_flag;

  /* freeze the mouse obligatory */
  freeze_mouse_flag = TRUE;

  /* get the active palette color */
  get_palette(pal);

  /* get a file name for the capture */
  for (c=0; c<10000; c++) {
    usprintf(buf, "shot%04d.%s", c, "pcx");
    if (!exists(buf))
      break;
  }

  if (ji_screen != screen)
    jmouse_draw_cursor();

  /* save in a bitmap the visible screen portion */
  bmp = create_sub_bitmap(ji_screen, 0, 0, JI_SCREEN_W, JI_SCREEN_H);
  save_bitmap(buf, bmp, pal);
  destroy_bitmap(bmp);

  /* restore the freeze flag by the previous value */
  freeze_mouse_flag = old_flag;
}
