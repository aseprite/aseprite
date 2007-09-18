/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>
#include <stdio.h>

#include "jinete.h"

int main (int argc, char *argv[])
{
  JWidget manager, window, box, label1, label2, label3, label4;

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

  window = jwindow_new ("Example 02");
  box = jbox_new (JI_VERTICAL | JI_HOMOGENEOUS);
  label1 = jlabel_new ("&Left Top");
  label2 = jlabel_new ("&Center Middle");
  label3 = jlabel_new ("&Right Bottom");
  label4 = jlabel_new ("&Disabled");

  jwidget_set_align (label1, JI_LEFT | JI_TOP);
  jwidget_set_align (label2, JI_CENTER | JI_MIDDLE);
  jwidget_set_align (label3, JI_RIGHT | JI_BOTTOM);
  jwidget_set_align (label4, JI_CENTER | JI_BOTTOM);

  jwidget_disable (label4);

  jwidget_add_child (window, box);
  jwidget_add_child (box, label1);
  jwidget_add_child (box, label2);
  jwidget_add_child (box, label3);
  jwidget_add_child (box, label4);

  jwindow_open_bg (window);

  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN();
