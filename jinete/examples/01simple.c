/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>

#include "jinete.h"

int main (int argc, char *argv[])
{
  JWidget manager, window, box1, box2, label, button_ok, button_cancel;

  /* Allegro stuff */
  allegro_init ();
  if (set_gfx_mode (GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
    allegro_message ("%s\n", allegro_error);
    return 1;
  }
  install_timer ();
  install_keyboard ();
  install_mouse ();

  /* Jinete initialization */
  manager = jmanager_new ();
  ji_set_standard_theme ();

  /* create main window */
  window = jwindow_new ("Example 01");
  box1 = jbox_new (JI_VERTICAL);
  box2 = jbox_new (JI_HORIZONTAL | JI_HOMOGENEOUS);
  label = jlabel_new ("A simple window");
  button_ok = jbutton_new ("&OK");
  button_cancel = jbutton_new ("&Cancel");

  jwidget_set_align (label, JI_CENTER | JI_MIDDLE);
  jwidget_expansive (label, TRUE);

  jwidget_add_child (window, box1);
  jwidget_add_child (box1, label);
  jwidget_add_child (box1, box2);
  jwidget_add_child (box2, button_ok);
  jwidget_add_child (box2, button_cancel);

  jwindow_open_bg (window);

  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN();
