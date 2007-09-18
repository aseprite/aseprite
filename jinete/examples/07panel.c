/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>

#include "jinete.h"

int main (int argc, char *argv[])
{
  JWidget manager, window, box1, box2, view1, view2, button;

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

  window = jwindow_new ("Example 07");
  box1 = jbox_new (JI_VERTICAL);
  box2 = ji_panel_new (JI_HORIZONTAL);
  view1 = jview_new ();
  view2 = jview_new ();
  button = jbutton_new ("&Close");

  jwidget_expansive (view1, TRUE);
  jwidget_expansive (view2, TRUE);
  jwidget_expansive (box2, TRUE);

  jwidget_add_child (window, box1);
  jwidget_add_childs (box1, box2, button, 0);
  jwidget_add_childs (box2, view1, view2, 0);

  jwindow_open_bg (window);

  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN ();
