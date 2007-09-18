/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>

#include "jinete.h"

int main (int argc, char *argv[])
{
  JWidget manager;
  int ret;

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

  jmanager_refresh_screen ();
  ji_mouse_set_cursor (JI_CURSOR_NORMAL);

  jalert ("Normal==First Alert||&Ok");
  jalert ("Error"
	  "==This is a long text line"
	  "--"
	  "<<Left text"
	  "==Centre text"
	  ">>Right text"
	  "||&Continue");

  do {
    ret = jalert ("Warning"
		  "<<Select some option:"
		  "--"
		  "<<Yes: first option"
		  "<<No: second option"
		  "<<Cancel: third option"
		  "--"
		  "||&Yes||&No||&Cancel");

  } while (jalert ("Progress"
		   "<<You select \"%s\""
		   "<<Are you sure?"
		   "||&Yeah||&Hmmm...",
		   ret == 1 ? "Yes":
		   ret == 2 ? "No":
		   ret == 3 ? "Cancel": "Nothing") != 1);

  jmanager_free (manager);
  return 0;
}

END_OF_MAIN ();
