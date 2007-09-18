/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>

#include "jinete.h"

static void poll_button (JWidget widget);

int main (int argc, char *argv[])
{
  JWidget manager, window;

  /* Allegro stuff */
  allegro_init ();
  if (set_gfx_mode (GFX_SAFE, 320, 200, 0, 0) < 0) {
    allegro_message ("%s\n", allegro_error);
    return 1;
  }
  install_timer ();
  install_keyboard ();
  install_mouse ();

  /* Jinete initialization */
  manager = jmanager_new ();
  ji_set_standard_theme ();

  window = ji_load_widget ("11file.jid", "Window1");
  if (window) {
    JWidget button_poll = jwidget_find_name (window, "Poll");
    jbutton_add_command (button_poll, poll_button);
    jwindow_open_bg (window);
  }
  else
    jalert ("Error loading main window||&OK");

  jmanager_run (manager);
  jmanager_free (manager);
  return 0;
}

END_OF_MAIN();

static void poll_button (JWidget widget)
{
  char filename[512], path[512];
  JWidget window;

  get_executable_name (path, sizeof (path));
  replace_filename (filename, path, "11file.jid", sizeof (filename));

  window = ji_load_widget (filename, "Window2");
  if (window) {
    JWidget button_select = jwidget_find_name (window, "Select");

    jwindow_open_fg (window);

    if (jwindow_get_killer (window) == button_select) {
      JWidget option1 = jwidget_find_name (window, "Option1");
      JWidget option2 = jwidget_find_name (window, "Option2");
      JWidget option3 = jwidget_find_name (window, "Option3");

      if (option1 && option2 && option3) {
	const char *msg = "";

	if (jwidget_is_selected (option1))
	  msg = "When you don't program it :-)";
	else if (jwidget_is_selected (option2))
	  msg =
	    "You lier, that is because"
	    "<<you don't see the Allegro one :-)";
	else if (jwidget_is_selected (option3))
	  msg = "You are blind :-)";
	else
	  msg = "Mhh... you must select a item";

	jalert ("Message from the Author<<%s||&Close", msg);
      }
    }

    jwidget_free (window);
  }
  else {
    jalert ("Error loading %s file||&OK", filename);
  }
}
