/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>

#include "jinete.h"

int main (int argc, char *argv[])
{
  JWidget manager1, manager2, manager3, window1, window2, window3;
  JWidget view2, view3;
  JRect rect;

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
  manager1 = jmanager_new ();
  ji_set_standard_theme ();
  manager2 = jmanager_new ();
  manager3 = jmanager_new ();

  window1 = jwindow_new ("Window1");
  window2 = jwindow_new ("Window2");
  window3 = jwindow_new ("Window3");
  view2 = jview_new ();
  view3 = jview_new ();

  jview_attach (view2, manager2);
  jview_attach (view3, manager3);

  jwidget_add_child (window1, view2);
  jwidget_add_child (window2, view3);

  jwindow_remap (window1);
  jwindow_remap (window2);
  jwindow_remap (window3);

  rect = jrect_new (JI_SCREEN_W/2-100, JI_SCREEN_H/2-70,
		      JI_SCREEN_W/2+100, JI_SCREEN_H/2+70);
  jwidget_set_rect (window1, rect);
  jrect_free (rect);

  rect = jrect_new (JI_SCREEN_W/2-90, JI_SCREEN_H/2-50,
		      JI_SCREEN_W/2+90, JI_SCREEN_H/2+50);
  jwidget_set_rect (window2, rect);
  jrect_free (rect);

  rect = jrect_new (JI_SCREEN_W/2-80, JI_SCREEN_H/2-30,
		      JI_SCREEN_W/2+80, JI_SCREEN_H/2+30);
  jwidget_set_rect (window3, rect);
  jrect_free (rect);

  jwindow_open_bg (window1);
  _jmanager_open_window(manager2, window2);
  _jmanager_open_window(manager3, window3);

  jmanager_run (manager1);
  jmanager_free (manager1);
  return 0;
}

END_OF_MAIN();
