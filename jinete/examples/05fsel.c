/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro.h>

#include "jinete.h"

static void show_file_select (const char *title, const char *exts);

int main (int argc, char *argv[])
{
  JWidget manager;

  /* Allegro stuff */
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

  show_file_select ("Select 'bmp,pcx'", "bmp,pcx");
  show_file_select ("Select '*t'", "*t");
  show_file_select ("Select 't*'", "t*");
  show_file_select ("Select '*'", "*");

  jmanager_free (manager);
  return 0;
}

END_OF_MAIN();

static void show_file_select (const char *title, const char *exts)
{
  char *filename = ji_file_select (title, "", exts);
  if (filename) {
    jalert ("Test<<You select the file:<<\"%s\"||&OK", filename);
    jfree (filename);
  }
}
