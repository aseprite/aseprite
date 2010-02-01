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

int main (int argc, char *argv[])
{
  JWidget manager, desktop, box, view, sub_manager, button;
  Window* subwindows[256];
  Window* window;
  char buf[256];
  int c;

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

  /* create desktop */
  desktop = jwindow_new_desktop();
  box = jbox_new(JI_VERTICAL);
  view = jview_new();
  sub_manager = jmanager_new();
  button = jbutton_new("&Close All");

  jview_attach(view, sub_manager);
  jwidget_expansive(view, true);

  jwidget_add_child(desktop, box);
  jwidget_add_child(box, view);
  jwidget_add_child(box, button);

  desktop->open_window_bg();

  /* generate 128 windows in random positions */
  for (c=0; c<128; c++) {
    usprintf(buf, "Window (%d)", c+1);

    window = jwindow_new(buf);
    button = jbutton_new("&Close");

    jwidget_add_child(window, button);

    window->remap_window();
    jwindow_position
      (window,
       sub_manager->rc->x1 + (rand () % (jrect_w(sub_manager->rc) - jrect_w(window->rc))),
       sub_manager->rc->y1 + (rand () % (jrect_h(sub_manager->rc) - jrect_h(window->rc))));

    _jmanager_open_window(sub_manager, window);
    subwindows[c] = window;
  }

  jmanager_run(manager);

  for (c=0; c<128; c++)
    jwidget_free(subwindows[c]);
  jwidget_free(desktop);
  
  jmanager_free(manager);
  return 0;
}

END_OF_MAIN();
