/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005  David A. Capello
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

#include <allegro/gfx.h>
#include <stdio.h>
#include <string.h>

#include "jinete/box.h"
#include "jinete/button.h"
#include "jinete/label.h"
#include "jinete/sep.h"
#include "jinete/system.h"
#include "jinete/widget.h"
#include "jinete/window.h"

#include "core/core.h"

#endif

void GUI_About(void)
{
  JWidget window, box1, label1, label2, label3, label4, label5;
  JWidget separator1, button1;

  if (!is_interactive())
    return;

  window = jwindow_new(_("About ASE"));
  box1 = jbox_new(JI_VERTICAL);
  label1 = jlabel_new("Allegro Sprite Editor - " VERSION);
  label2 = jlabel_new(_("The Ultimate Sprites Factory"));
  separator1 = ji_separator_new(NULL, JI_HORIZONTAL);
  label3 = jlabel_new("Copyright (C) 2001-2005");
  label4 = jlabel_new(_("by David A. Capello"));
  label5 = jlabel_new("http://ase.sourceforge.net/");
  button1 = jbutton_new(_("&Close"));

  jwidget_magnetic(button1, TRUE);

  jwidget_add_childs(box1, label1, label2, separator1,
		     label3, label4, label5, button1, NULL);
  jwidget_add_child(window, box1);

  jwindow_open_fg(window);
  jwidget_free(window);
}
