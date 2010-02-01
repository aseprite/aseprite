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
  JWidget manager1, manager2, manager3;
  Window* window1, *window2, *window3;
  JWidget view2, view3;
  JRect rect;

  /* Allegro stuff */
  allegro_init ();
  if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0) < 0) {
    if (set_gfx_mode(GFX_AUTODETECT, 320, 200, 0, 0) < 0) {
      allegro_message("%s\n", allegro_error);
      return 1;
    }
  }
  install_timer ();
  install_keyboard ();
  install_mouse ();

  /* Jinete initialization */
  manager1 = jmanager_new();
  ji_set_standard_theme();
  manager2 = jmanager_new();
  manager3 = jmanager_new();

  window1 = jwindow_new("Window1");
  window2 = jwindow_new("Window2");
  window3 = jwindow_new("Window3");
  view2 = jview_new();
  view3 = jview_new();

  jview_attach(view2, manager2);
  jview_attach(view3, manager3);

  jwidget_add_child(window1, view2);
  jwidget_add_child(window2, view3);

  window1->remap_window();
  window2->remap_window();
  window3->remap_window();

  rect = jrect_new(JI_SCREEN_W/2-100, JI_SCREEN_H/2-70,
		   JI_SCREEN_W/2+100, JI_SCREEN_H/2+70);
  jwidget_set_rect(window1, rect);
  jrect_free(rect);

  rect = jrect_new(JI_SCREEN_W/2-90, JI_SCREEN_H/2-50,
		   JI_SCREEN_W/2+90, JI_SCREEN_H/2+50);
  jwidget_set_rect(window2, rect);
  jrect_free(rect);

  rect = jrect_new(JI_SCREEN_W/2-80, JI_SCREEN_H/2-30,
		   JI_SCREEN_W/2+80, JI_SCREEN_H/2+30);
  jwidget_set_rect(window3, rect);
  jrect_free(rect);

  window1->open_window_bg();
  _jmanager_open_window(manager2, window2);
  _jmanager_open_window(manager3, window3);

  jmanager_run(manager1);
  jmanager_free(manager1);
  return 0;
}

END_OF_MAIN();
