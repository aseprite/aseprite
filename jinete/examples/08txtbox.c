/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>

#include "jinete.h"

void do_text (const char *title, int align)
{
  const char *LGPL_text =
    "This library is free software; you can redistribute it and/or\n"
    "modify it under the terms of the GNU Lesser General Public\n"
    "License as published by the Free Software Foundation; either\n"
    "version 2.1 of the License, or (at your option) any later version.\n"
    "\n"
    "This library is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
    "Lesser General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU Lesser General Public\n"
    "License along with this library; if not, write to the Free Software\n"
    "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA";

  JWidget window, box1, box2, box3, box4, label1, label2;
  JWidget view1, view2, text_box1, text_box2, button;

  window = jwindow_new (title);
  box1 = jbox_new (JI_VERTICAL);
  box2 = jbox_new (JI_HORIZONTAL | JI_HOMOGENEOUS);
  box3 = jbox_new (JI_VERTICAL);
  box4 = jbox_new (JI_VERTICAL);
  label1 = jlabel_new ("With word-wrap");
  label2 = jlabel_new ("Without word-wrap");
  view1 = jview_new ();
  view2 = jview_new ();
  text_box1 = jtextbox_new (LGPL_text, align | JI_WORDWRAP);
  text_box2 = jtextbox_new (LGPL_text, align | 0);
  button = jbutton_new ("&Close");

  jview_attach (view1, text_box1);
  jview_attach (view2, text_box2);

  jwidget_expansive (view1, TRUE);
  jwidget_expansive (view2, TRUE);
  jwidget_expansive (box2, TRUE);

  jwidget_set_static_size (view1, 64, 64);
  jwidget_set_static_size (view2, 64, 64);

  jwidget_add_child (box3, label1);
  jwidget_add_child (box3, view1);
  jwidget_add_child (box4, label2);
  jwidget_add_child (box4, view2);
  jwidget_add_child (box2, box3);
  jwidget_add_child (box2, box4);
  jwidget_add_child (box1, box2);
  jwidget_add_child (box1, button);
  jwidget_add_child (window, box1);

  jwindow_open_bg (window);
}

int main (int argc, char *argv[])
{
  JWidget manager;

  allegro_init ();
  if (set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
    allegro_message ("%s\n", allegro_error);
    return 1;
  }
  install_timer ();
  install_keyboard ();
  install_mouse ();

  manager = jmanager_new ();
  ji_set_standard_theme ();

  do_text ("Right", JI_RIGHT);
  do_text ("Center", JI_CENTER);
  do_text ("Left", JI_LEFT);

  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN ();
