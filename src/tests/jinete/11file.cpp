/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the Jinete nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <allegro.h>

#include "jinete/jinete.h"

static void poll_button (JWidget widget);

int main (int argc, char *argv[])
{
  JWidget manager, window;

  /* Allegro stuff */
  allegro_init ();
  if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0) < 0) {
    if (set_gfx_mode(GFX_AUTODETECT, 640, 480, 0, 0) < 0) {
      allegro_message("%s\n", allegro_error);
      return 1;
    }
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
    window->open_window_bg();
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

    window->open_window_fg();

    if (window->get_killer() == button_select) {
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
