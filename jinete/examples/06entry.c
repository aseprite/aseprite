/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>

#include "jinete.h"

int main (int argc, char *argv[])
{
  JWidget manager, window1, box1, box2, box3, box4, box5;
  JWidget label1, label2, entry1, entry2;
  JWidget button1, button2;

  allegro_init ();
  set_color_depth (8);
  if (set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
    allegro_message ("%s\n", allegro_error);
    return 1;
  }
  install_timer ();
  install_keyboard ();
  install_mouse ();

  manager = jmanager_new ();
  ji_set_standard_theme ();

  window1 = jwindow_new ("Example 06");
  box1 = jbox_new (JI_VERTICAL);
  box2 = jbox_new (JI_HORIZONTAL);
  box3 = jbox_new (JI_VERTICAL | JI_HOMOGENEOUS);
  box4 = jbox_new (JI_VERTICAL | JI_HOMOGENEOUS);
  box5 = jbox_new (JI_HORIZONTAL | JI_HOMOGENEOUS);
  label1 = jlabel_new ("Entry1:");
  label2 = jlabel_new ("Entry2:");
  entry1 = jentry_new (8, "");
  entry2 = jentry_new (256, "%d+%d=%d", 4, 5, 4+5);
  button1 = jbutton_new ("&OK");
  button2 = jbutton_new ("&Cancel");

  jwidget_expansive (box4, TRUE);
  jwidget_expansive (box5, TRUE);
  jwidget_magnetic (button1, TRUE);

  /* hierarchy */
  jwidget_add_child (window1, box1);
    jwidget_add_child (box1, box2);
      jwidget_add_child (box2, box3);
        jwidget_add_child (box3, label1);
        jwidget_add_child (box3, label2);
      jwidget_add_child (box2, box4);
        jwidget_add_child (box4, entry1);
        jwidget_add_child (box4, entry2);
    jwidget_add_child (box1, box5);
      jwidget_add_child (box5, button1);
      jwidget_add_child (box5, button2);

  jwindow_open_bg (window1);
  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN();
