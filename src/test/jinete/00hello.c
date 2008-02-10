/* jinete - a GUI library
 * Copyright (C) 2003-2008 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>

#include "jinete/jinete.h"

int main(int argc, char *argv[])
{
  /* widgets to use */
  JWidget manager, window, box, label, button;

  /* Allegro stuff */
  allegro_init();
  if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0) < 0) {
    if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
      allegro_message("%s\n", allegro_error);
      return 1;
    }
  }
  install_timer();
  install_keyboard();
  install_mouse();

  /* Jinete initialization */
  manager = jmanager_new();
  ji_set_standard_theme();

  /* create main window */
  window = jwindow_new("Example 00");

  /* create a vertical box to set label and button positions */
  box = jbox_new(JI_VERTICAL);

  /* create a simple centered label with "Hello World!" text */
  label = jlabel_new("Hello World!");
  jwidget_set_align(label, JI_CENTER | JI_MIDDLE);

  /* create a button to close the window */
  button = jbutton_new("&Close");

  /* a expansive widget can get more space that other ones */
  jwidget_expansive(label, TRUE);

  /* put the label and button in the box and that box in the window */
  jwidget_add_child(box, label);
  jwidget_add_child(box, button);
  jwidget_add_child(window, box);

  /* show the window in the screen and leave it in the background (the
     manager will free it) */
  jwindow_open_bg(window);

  /* run windows */
  jmanager_run(manager);

  /* Jinete finalization */
  jmanager_free(manager);
  return 0;
}

END_OF_MAIN();
