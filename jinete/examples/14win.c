/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>
#include "jinete.h"

int main (int argc, char *argv[])
{
  JWidget manager, desktop, box, view, sub_manager, button, window;
  char buf[256];
  int c;

  /* Allegro stuff */
  allegro_init();
  if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) < 0) {
    allegro_message("%s\n", allegro_error);
    return 1;
  }
  install_timer();
  install_keyboard();
  install_mouse();

  /* Jinete initialization */
  manager = jmanager_new();
  ji_set_standard_theme();

  /* create desktop */
  desktop = jwindow_new_desktop();
  box = jbox_new(JI_VERTICAL);
  view = jview_new();
  sub_manager = jmanager_new();
  button = jbutton_new("&Close All");

  jview_attach(view, sub_manager);
  jwidget_expansive(view, TRUE);

  jwidget_add_child (desktop, box);
  jwidget_add_child (box, view);
  jwidget_add_child (box, button);

  jwindow_open_bg(desktop);

  /* generate 128 windows in random positions */
  for (c=0; c<128; c++) {
    usprintf(buf, "Window (%d)", c+1);

    window = jwindow_new(buf);
    button = jbutton_new("&Close");

    jwidget_add_child (window, button);
    jwidget_autodestroy(window, TRUE);

    jwindow_remap(window);
    jwindow_position
      (window,
       sub_manager->rc->x1 + (rand () % (jrect_w(sub_manager->rc) - jrect_w(window->rc))),
       sub_manager->rc->y1 + (rand () % (jrect_h(sub_manager->rc) - jrect_h(window->rc))));

    _jmanager_open_window (sub_manager, window);
  }

  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN();
