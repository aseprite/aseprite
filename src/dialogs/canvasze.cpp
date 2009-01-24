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

#include "config.h"

#include <allegro.h>

#include "jinete/jbox.h"
#include "jinete/jbutton.h"
#include "jinete/jentry.h"
#include "jinete/jhook.h"
#include "jinete/jlabel.h"
#include "jinete/jwidget.h"
#include "jinete/jwindow.h"

#include "core/cfg.h"
#include "core/color.h"
#include "core/core.h"
/* #include "modules/editors.h" */
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/sprites.h"
#include "raster/sprite.h"
#include "widgets/colbut.h"
#include "widgets/groupbut.h"

void canvas_resize()
{
  JWidget window, box1, box2, box3, box4, box5, box6;
  JWidget label_w, label_h;
  JWidget entry_w, entry_h;
  JWidget check_w, check_h;
  JWidget button_offset;
  JWidget button_ok, button_cancel;
  Sprite *sprite = current_sprite;

  if (!is_interactive () || !sprite)
    return;

  window = jwindow_new(_("Canvas Size"));
  box1 = jbox_new(JI_VERTICAL);
  box2 = jbox_new(JI_HORIZONTAL);
  box3 = jbox_new(JI_VERTICAL | JI_HOMOGENEOUS);
  box4 = jbox_new(JI_VERTICAL | JI_HOMOGENEOUS);
  box5 = jbox_new(JI_VERTICAL | JI_HOMOGENEOUS);
  box6 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);
  label_w = jlabel_new(_("Width:"));
  label_h = jlabel_new(_("Height:"));
  entry_w = jentry_new(8, "%d", sprite->w);
  entry_h = jentry_new(8, "%d", sprite->h);
  check_w = jcheck_new("%");
  check_h = jcheck_new("%");
  button_offset = group_button_new(3, 3, 4,
				   -1, -1, -1,
				   -1, -1, -1,
				   -1, -1, -1);
  button_ok = jbutton_new(_("&OK"));
  button_cancel = jbutton_new(_("&Cancel"));

  jwidget_magnetic(button_ok, TRUE);

  jwidget_expansive(box2, TRUE);
  jwidget_expansive(button_offset, TRUE);

  jwidget_add_child(box3, label_w);
  jwidget_add_child(box3, label_h);
  jwidget_add_child(box4, entry_w);
  jwidget_add_child(box4, entry_h);
  jwidget_add_child(box5, check_w);
  jwidget_add_child(box5, check_h);
  jwidget_add_child(box2, box3);
  jwidget_add_child(box2, box4);
  jwidget_add_child(box2, box5);
  jwidget_add_child(box2, button_offset);
  jwidget_add_child(box1, box2);
  jwidget_add_child(box6, button_ok);
  jwidget_add_child(box6, button_cancel);
  jwidget_add_child(box1, box6);
  jwidget_add_child(window, box1);

  /* default position */
  jwindow_remap(window);
  jwindow_center(window);

  /* load window configuration */
  load_window_pos(window, "CanvasSize");

  /* open the window */
  jwindow_open_fg(window);

  if (jwindow_get_killer(window) == button_ok) {
    /* update editors */
    update_screen_for_sprite(sprite);
  }

  /* save window configuration */
  save_window_pos(window, "CanvasSize");

  jwidget_free(window);
}
