/* Jinete - a GUI library
 * Copyright (c) 2003, 2004, 2005, 2007, David A. Capello
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

#include "jinete.h"

int main(int argc, char *argv[])
{
  JWidget manager, window, box;
  JWidget slider1, slider2, slider3, slider4, slider5, button1;

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

  window = jwindow_new("Example 03");
  box = jbox_new(JI_VERTICAL);
  slider1 = jslider_new(0, 1, 0);
  slider2 = jslider_new(0, 2, 0);
  slider3 = jslider_new(0, 10, 0);
  slider4 = jslider_new(0, 63, 1);
  slider5 = jslider_new(0, 255, 1);
  button1 = jbutton_new("&Close");

  jwidget_set_static_size(slider1, 128, 0);
  jwidget_set_static_size(slider2, 128, 0);
  jwidget_set_static_size(slider3, 128, 0);
  jwidget_set_static_size(slider4, 128, 0);
  jwidget_set_static_size(slider5, 128, 0);

  jwidget_expansive(slider1, TRUE);
  jwidget_expansive(slider2, TRUE);
  jwidget_expansive(slider3, TRUE);
  jwidget_expansive(slider4, TRUE);
  jwidget_expansive(slider5, TRUE);

  jwidget_add_child(window, box);
  jwidget_add_childs(box,
		     slider1, slider2, slider3,
		     slider4, slider5, button1, NULL);

  jwindow_open_bg(window);

  jmanager_run(manager);
  jmanager_free(manager);
  return 0;
}

END_OF_MAIN();
