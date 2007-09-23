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

#include "jinete.h"

#include "core/core.h"

#endif

void command_execute_about(const char *argument)
{
  JWidget window, box1, label1, label2, label3, label4;
  JWidget separator1, button1;

  window = jwindow_new(_("About ASE"));
  box1 = jbox_new(JI_VERTICAL);
  label1 = jlabel_new("Allegro Sprite Editor - " VERSION);
  label2 = jlabel_new(_("The Ultimate Sprites Factory"));
  separator1 = ji_separator_new(NULL, JI_HORIZONTAL);
  label3 = jlabel_new(COPYRIGHT);
  label4 = jlabel_new("http://ase.sourceforge.net/");
  button1 = jbutton_new(_("&Close"));

  jwidget_magnetic(button1, TRUE);

  jwidget_add_childs(box1, label1, label2, separator1, label3, label4,
		     button1, NULL);
  jwidget_add_child(window, box1);

  jwindow_open_fg(window);
  jwidget_free(window);
}
